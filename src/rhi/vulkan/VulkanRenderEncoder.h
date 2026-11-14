#pragma once

/**
 * @file
 * @brief rhi::vulkan::VulkanRenderEncoder: records draws inside a VkRenderPass instance.
 */

#include <memory>

#include "IRenderEncoder.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanCommandBuffer;

/**
 * @brief The Vulkan IRenderEncoder. Created by VulkanCommandBuffer::beginRender, which has already begun the
 * render pass.
 *
 * @details
 * Pipelines, bind groups, vertex and index buffers bound here are retained by the command buffer.
 * setBindGroup() binds through the current pipeline's layout, so it records nothing before setPipeline().
 */
class VulkanRenderEncoder final : public IRenderEncoder {
public:
    /**
     * @brief Records into @p command_buffer, which it keeps alive.
     *
     * @param command_buffer The command buffer, in the Encoding state, inside a begun render pass.
     */
    explicit VulkanRenderEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer);
    /**
     * @brief Ends the render pass if end() was not called.
     */
    ~VulkanRenderEncoder() override;

    void setPipeline(PipelineRef pipeline) override;
    void setBindGroup(BindGroupBinding binding) override;
    void setVertexBuffers(VertexBufferBindings bindings) override;
    void setIndexBuffer(IndexBufferBinding binding) override;
    void draw(DrawDesc desc) override;
    void drawIndexed(DrawIndexedDesc desc) override;
    /**
     * @brief Records vkCmdEndRenderPass and returns the command buffer to the Recording state.
     */
    void end() override;
private:
    std::shared_ptr<VulkanCommandBuffer> m_command_buffer;
    PipelineRef m_pipeline;
    bool m_ended = false;
    VkCommandBuffer m_native_commands = VK_NULL_HANDLE;
};

} // namespace rhi::vulkan
