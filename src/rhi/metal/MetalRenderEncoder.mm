/**
 * @file
 * @brief MetalRenderEncoder: render pass recording.
 */

#include "MetalRenderEncoder.h"

#include <utility>

#include "MetalBuffer.h"
#include "MetalCommandBuffer.h"
#include "MetalPipeline.h"

namespace rhi::metal {

MetalRenderEncoder::MetalRenderEncoder(std::shared_ptr<MetalCommandBuffer> command_buffer,
                                       id<MTLRenderCommandEncoder> encoder)
    : m_command_buffer(std::move(command_buffer)), m_encoder(encoder) {
}

MetalRenderEncoder::~MetalRenderEncoder() {
    end();
}

void MetalRenderEncoder::setPipeline(PipelineRef pipeline) {
    if (m_ended || !pipeline) {
        return;
    }
    const auto& metal = static_cast<const MetalPipeline&>(*pipeline);
    if (metal.getKind() != PipelineKind::Graphics) {
        m_command_buffer->reportError("IRenderEncoder::setPipeline: not a graphics pipeline");
        return;
    }
    [m_encoder setRenderPipelineState:metal.m_render_state];
    [m_encoder setDepthStencilState:metal.m_depth_stencil_state];
    // Metal sets the raster state on the encoder rather than in the pipeline state.
    [m_encoder setCullMode:metal.m_cull_mode];
    [m_encoder setFrontFacingWinding:metal.m_front_face];
    [m_encoder setTriangleFillMode:metal.m_fill_mode];
    m_pipeline = std::static_pointer_cast<const MetalPipeline>(pipeline);
    m_command_buffer->retain(std::move(pipeline));
}

void MetalRenderEncoder::setBindGroup(BindGroupBinding) {
    m_command_buffer->reportError("MetalRenderEncoder::setBindGroup is not implemented yet");
}

void MetalRenderEncoder::setVertexBuffers(VertexBufferBindings bindings) {
    if (m_ended) {
        return;
    }
    for (std::size_t i = 0; i < bindings.buffers.size(); ++i) {
        VertexBufferBinding& binding = bindings.buffers[i];
        const std::uint32_t slot = bindings.first_slot + static_cast<std::uint32_t>(i);
        if (!binding.buffer || slot >= 8) {
            m_command_buffer->reportError("IRenderEncoder::setVertexBuffers: null buffer or slot >= 8");
            continue;
        }
        [m_encoder setVertexBuffer:static_cast<const MetalBuffer&>(*binding.buffer).handle()
                            offset:binding.offset
                           atIndex:kVertexBufferBase - slot];
        m_command_buffer->retain(std::move(binding.buffer));
    }
}

void MetalRenderEncoder::setIndexBuffer(IndexBufferBinding binding) {
    if (m_ended || !binding.buffer) {
        return;
    }
    // Metal takes the index buffer with each indexed draw; remember it until then.
    m_index_buffer = static_cast<const MetalBuffer&>(*binding.buffer).handle();
    m_index_offset = binding.offset;
    m_index_type = binding.format == IndexFormat::Uint16 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
    m_command_buffer->retain(std::move(binding.buffer));
}

void MetalRenderEncoder::draw(DrawDesc desc) {
    if (m_ended || !m_pipeline) {
        return;
    }
    [m_encoder drawPrimitives:m_pipeline->m_primitive_type
                  vertexStart:desc.first_vertex
                  vertexCount:desc.vertex_count
                instanceCount:desc.instance_count
                 baseInstance:desc.first_instance];
}

void MetalRenderEncoder::drawIndexed(DrawIndexedDesc desc) {
    if (m_ended || !m_pipeline || m_index_buffer == nil) {
        return;
    }
    const NSUInteger index_size = m_index_type == MTLIndexTypeUInt16 ? 2 : 4;
    [m_encoder drawIndexedPrimitives:m_pipeline->m_primitive_type
                          indexCount:desc.index_count
                           indexType:m_index_type
                         indexBuffer:m_index_buffer
                   indexBufferOffset:m_index_offset + desc.first_index * index_size
                       instanceCount:desc.instance_count
                          baseVertex:desc.vertex_offset
                        baseInstance:desc.first_instance];
}

void MetalRenderEncoder::end() {
    if (m_ended) {
        return;
    }
    m_ended = true;
    [m_encoder endEncoding];
    m_command_buffer->endEncoder();
}

} // namespace rhi::metal
