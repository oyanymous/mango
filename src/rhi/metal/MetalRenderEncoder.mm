#include "MetalRenderEncoder.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include "MetalBindGroup.h"
#include "MetalBuffer.h"
#include "MetalCommandBuffer.h"
#include "MetalPipeline.h"

namespace rhi::metal {

void MetalRenderState::setPipelineState(id<MTLRenderPipelineState> state) {
    if (state != m_pipeline_state) {
        [m_encoder setRenderPipelineState:state];
        m_pipeline_state = state;
    }
}

void MetalRenderState::setDepthStencilState(id<MTLDepthStencilState> state) {
    if (state != m_depth_stencil_state) {
        [m_encoder setDepthStencilState:state];
        m_depth_stencil_state = state;
    }
}

void MetalRenderState::setRaster(MTLCullMode cull, MTLWinding winding, MTLTriangleFillMode fill) {
    if (!m_raster_set || cull != m_cull) {
        [m_encoder setCullMode:cull];
    }
    if (!m_raster_set || winding != m_winding) {
        [m_encoder setFrontFacingWinding:winding];
    }
    if (!m_raster_set || fill != m_fill) {
        [m_encoder setTriangleFillMode:fill];
    }
    m_raster_set = true;
    m_cull = cull;
    m_winding = winding;
    m_fill = fill;
}

void MetalRenderState::setStencilReference(std::uint32_t reference) {
    if (!m_stencil_reference_set || reference != m_stencil_reference) {
        [m_encoder setStencilReferenceValue:reference];
        m_stencil_reference_set = true;
        m_stencil_reference = reference;
    }
}

void MetalRenderState::setBuffer(ShaderStage which, id<MTLBuffer> buffer, NSUInteger offset, NSUInteger index) {
    if (index >= kBuffers) {
        return;
    }
    StageState& state = stage(which);
    if (state.buffers[index] == buffer && state.offsets[index] == offset) {
        return;
    }
    if (state.buffers[index] == buffer) {
        if (which == ShaderStage::Vertex) {
            [m_encoder setVertexBufferOffset:offset atIndex:index];
        } else {
            [m_encoder setFragmentBufferOffset:offset atIndex:index];
        }
    } else if (which == ShaderStage::Vertex) {
        [m_encoder setVertexBuffer:buffer offset:offset atIndex:index];
    } else {
        [m_encoder setFragmentBuffer:buffer offset:offset atIndex:index];
    }
    state.buffers[index] = buffer;
    state.offsets[index] = offset;
}

void MetalRenderState::setTexture(ShaderStage which, id<MTLTexture> texture, NSUInteger index) {
    StageState& state = stage(which);
    if (index >= kTextures || state.textures[index] == texture) {
        return;
    }
    if (which == ShaderStage::Vertex) {
        [m_encoder setVertexTexture:texture atIndex:index];
    } else {
        [m_encoder setFragmentTexture:texture atIndex:index];
    }
    state.textures[index] = texture;
}

void MetalRenderState::setSampler(ShaderStage which, id<MTLSamplerState> sampler, NSUInteger index) {
    StageState& state = stage(which);
    if (index >= kSamplers || state.samplers[index] == sampler) {
        return;
    }
    if (which == ShaderStage::Vertex) {
        [m_encoder setVertexSamplerState:sampler atIndex:index];
    } else {
        [m_encoder setFragmentSamplerState:sampler atIndex:index];
    }
    state.samplers[index] = sampler;
}

MetalRenderEncoder::MetalRenderEncoder(std::shared_ptr<MetalCommandBuffer> command_buffer,
                                       id<MTLRenderCommandEncoder> encoder, Extent2D extent)
    : m_command_buffer(std::move(command_buffer)),
      m_encoder(encoder),
      m_state(encoder),
      m_extent(extent),
      m_viewport{0.0, 0.0, static_cast<double>(extent.width), static_cast<double>(extent.height), 0.0, 1.0},
      m_scissor{0, 0, extent.width, extent.height} {
}

void MetalRenderEncoder::flushState() {
    if (m_viewport_dirty) {
        [m_encoder setViewport:m_viewport];
        m_viewport_dirty = false;
    }
    if (m_scissor_dirty) {
        [m_encoder setScissorRect:m_scissor];
        m_scissor_dirty = false;
    }
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
    m_state.setPipelineState(metal.m_render_state);
    m_state.setDepthStencilState(metal.m_depth_stencil_state);

    m_state.setRaster(metal.m_cull_mode, metal.m_front_face, metal.m_fill_mode);
    m_pipeline = std::static_pointer_cast<const MetalPipeline>(pipeline);
    m_command_buffer->retain(std::move(pipeline));
}

void MetalRenderEncoder::setBindGroup(BindGroupBinding binding) {
    if (m_ended || !binding.bind_group) {
        return;
    }
    if (!m_pipeline) {
        m_command_buffer->reportError("IRenderEncoder::setBindGroup: set a pipeline first");
        return;
    }
    static_cast<const MetalBindGroup&>(*binding.bind_group).bind(m_state, m_pipeline->layout(), binding.index);
    m_command_buffer->retain(std::move(binding.bind_group));
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
        m_state.setBuffer(ShaderStage::Vertex, static_cast<const MetalBuffer&>(*binding.buffer).handle(),
                          binding.offset, kVertexBufferBase - slot);
        m_command_buffer->retain(std::move(binding.buffer));
    }
}

void MetalRenderEncoder::setIndexBuffer(IndexBufferBinding binding) {
    if (m_ended || !binding.buffer) {
        return;
    }

    m_index_buffer = static_cast<const MetalBuffer&>(*binding.buffer).handle();
    m_index_offset = binding.offset;
    m_index_type = binding.format == IndexFormat::Uint16 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
    m_command_buffer->retain(std::move(binding.buffer));
}

void MetalRenderEncoder::draw(DrawDesc desc) {
    if (m_ended || !m_pipeline || m_scissor_empty) {
        return;
    }
    flushState();
    [m_encoder drawPrimitives:m_pipeline->m_primitive_type
                  vertexStart:desc.first_vertex
                  vertexCount:desc.vertex_count
                instanceCount:desc.instance_count
                 baseInstance:desc.first_instance];
}

void MetalRenderEncoder::drawIndexed(DrawIndexedDesc desc) {
    if (m_ended || !m_pipeline || m_index_buffer == nil || m_scissor_empty) {
        return;
    }
    flushState();
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

void MetalRenderEncoder::setViewport(Viewport viewport) {
    if (m_ended) {
        return;
    }
    m_viewport =
        MTLViewport{viewport.x, viewport.y, viewport.width, viewport.height, viewport.min_depth, viewport.max_depth};
    m_viewport_dirty = true;
}

void MetalRenderEncoder::setScissor(ScissorRect rect) {
    if (m_ended) {
        return;
    }

    const std::int64_t width = m_extent.width;
    const std::int64_t height = m_extent.height;
    const std::int64_t x0 = std::clamp<std::int64_t>(rect.x, 0, width);
    const std::int64_t y0 = std::clamp<std::int64_t>(rect.y, 0, height);
    const std::int64_t x1 = std::clamp<std::int64_t>(static_cast<std::int64_t>(rect.x) + rect.width, 0, width);
    const std::int64_t y1 = std::clamp<std::int64_t>(static_cast<std::int64_t>(rect.y) + rect.height, 0, height);
    m_scissor_empty = x1 <= x0 || y1 <= y0;
    if (m_scissor_empty) {
        return;
    }
    const MTLScissorRect scissor{static_cast<NSUInteger>(x0), static_cast<NSUInteger>(y0),
                                 static_cast<NSUInteger>(x1 - x0), static_cast<NSUInteger>(y1 - y0)};
    if (scissor.x != m_scissor.x || scissor.y != m_scissor.y || scissor.width != m_scissor.width ||
        scissor.height != m_scissor.height) {
        m_scissor = scissor;
        m_scissor_dirty = true;
    }
}

void MetalRenderEncoder::setStencilReference(std::uint32_t reference) {
    if (m_ended) {
        return;
    }
    m_state.setStencilReference(reference & 0xffu);
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
