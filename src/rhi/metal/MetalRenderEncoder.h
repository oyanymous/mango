#pragma once

/**
 * @file
 * @brief rhi::metal::MetalRenderEncoder: draws on an `MTLRenderCommandEncoder`.
 */

#include <memory>

#include "IRenderEncoder.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalCommandBuffer;

/**
 * @brief Records one render pass. Ends itself when destroyed.
 */
class MetalRenderEncoder final : public IRenderEncoder {
public:
    /**
     * @brief Wraps an open render encoder. Created by MetalCommandBuffer::beginRender.
     *
     * @param command_buffer The command buffer it records into; retains the resources used.
     * @param encoder The open render encoder.
     */
    MetalRenderEncoder(std::shared_ptr<MetalCommandBuffer> command_buffer, id<MTLRenderCommandEncoder> encoder);
    ~MetalRenderEncoder() override;

    void setPipeline(PipelineRef pipeline) override;
    void setBindGroup(BindGroupBinding binding) override;
    void setVertexBuffers(VertexBufferBindings bindings) override;
    void setIndexBuffer(IndexBufferBinding binding) override;
    void draw(DrawDesc desc) override;
    void drawIndexed(DrawIndexedDesc desc) override;
    void end() override;
private:
    std::shared_ptr<MetalCommandBuffer> m_command_buffer;
    id<MTLRenderCommandEncoder> m_encoder = nil;
    bool m_ended = false;
};

} // namespace rhi::metal
