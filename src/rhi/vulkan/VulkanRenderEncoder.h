#pragma once

#include <memory>

#include "IRenderEncoder.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanCommandBuffer;

class VulkanRenderEncoder final : public IRenderEncoder {
public:
    VulkanRenderEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer, Extent2D extent);

    ~VulkanRenderEncoder() override;

    void setPipeline(PipelineRef pipeline) override;
    void setBindGroup(BindGroupBinding binding) override;
    void setVertexBuffers(VertexBufferBindings bindings) override;
    void setIndexBuffer(IndexBufferBinding binding) override;
    void draw(DrawDesc desc) override;
    void drawIndexed(DrawIndexedDesc desc) override;
    void setViewport(Viewport viewport) override;
    void setScissor(ScissorRect rect) override;
    void setStencilReference(std::uint32_t reference) override;

    void end() override;
private:
    std::shared_ptr<VulkanCommandBuffer> m_command_buffer;
    PipelineRef m_pipeline;
    bool m_ended = false;
    Extent2D m_extent;
    bool m_scissor_empty = false;
    const vk::raii::CommandBuffer& m_commands;
};

} // namespace rhi::vulkan
