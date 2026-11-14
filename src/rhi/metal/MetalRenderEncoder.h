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
class MetalPipeline;

/**
 * @brief Records one render pass. Ends itself when destroyed.
 *
 * @details
 * Vertex buffer slot N binds to Metal buffer index `kVertexBufferBase - N`. Draws need a pipeline, and indexed draws
 * an index buffer, set first; otherwise they are skipped. Bind groups bind through the current pipeline's layout, so
 * set the pipeline before them (as in the Vulkan backend).
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
    std::shared_ptr<const MetalPipeline> m_pipeline; // the current pipeline (primitive type, binding remap)
    id<MTLBuffer> m_index_buffer = nil;              // set by setIndexBuffer, used by drawIndexed
    std::uint64_t m_index_offset = 0;
    MTLIndexType m_index_type = MTLIndexTypeUInt32;
    bool m_ended = false;
};

} // namespace rhi::metal
