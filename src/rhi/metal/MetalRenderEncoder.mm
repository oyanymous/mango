/**
 * @file
 * @brief MetalRenderEncoder: render pass recording.
 */

#include "MetalRenderEncoder.h"

#include <utility>

#include "MetalCommandBuffer.h"

namespace rhi::metal {

MetalRenderEncoder::MetalRenderEncoder(std::shared_ptr<MetalCommandBuffer> command_buffer,
                                       id<MTLRenderCommandEncoder> encoder)
    : m_command_buffer(std::move(command_buffer)), m_encoder(encoder) {
}

MetalRenderEncoder::~MetalRenderEncoder() {
    end();
}

void MetalRenderEncoder::setPipeline(PipelineRef) {
    m_command_buffer->reportError("MetalRenderEncoder::setPipeline is not implemented yet");
}

void MetalRenderEncoder::setBindGroup(BindGroupBinding) {
    m_command_buffer->reportError("MetalRenderEncoder::setBindGroup is not implemented yet");
}

void MetalRenderEncoder::setVertexBuffers(VertexBufferBindings) {
    m_command_buffer->reportError("MetalRenderEncoder::setVertexBuffers is not implemented yet");
}

void MetalRenderEncoder::setIndexBuffer(IndexBufferBinding) {
    m_command_buffer->reportError("MetalRenderEncoder::setIndexBuffer is not implemented yet");
}

void MetalRenderEncoder::draw(DrawDesc) {
    m_command_buffer->reportError("MetalRenderEncoder::draw is not implemented yet");
}

void MetalRenderEncoder::drawIndexed(DrawIndexedDesc) {
    m_command_buffer->reportError("MetalRenderEncoder::drawIndexed is not implemented yet");
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
