#pragma once

/**
 * @file
 * @brief rhi::vulkan::VulkanComputeEncoder: records compute dispatches.
 */

#include <memory>

#include "IComputeEncoder.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanCommandBuffer;

/**
 * @brief The Vulkan IComputeEncoder. Created by VulkanCommandBuffer::beginCompute.
 *
 * @details
 * Pipelines and bind groups bound here are retained by the command buffer. setBindGroup() binds through the
 * current pipeline's layout, so it records nothing before setPipeline(). Synchronizing dispatches with each
 * other or with later commands is the caller's job, with ICommandBuffer::barrier between encoders.
 */
class VulkanComputeEncoder final : public IComputeEncoder {
public:
    /**
     * @brief Records into @p command_buffer, which it keeps alive.
     *
     * @param command_buffer The command buffer, in the Encoding state.
     */
    explicit VulkanComputeEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer);
    /**
     * @brief Ends the encoder if end() was not called.
     */
    ~VulkanComputeEncoder() override;

    void setPipeline(PipelineRef pipeline) override;
    void setBindGroup(BindGroupBinding binding) override;
    void dispatch(DispatchDesc desc) override;
    /**
     * @brief Returns the command buffer to the Recording state.
     */
    void end() override;
private:
    std::shared_ptr<VulkanCommandBuffer> m_command_buffer;
    PipelineRef m_pipeline;
    bool m_ended = false;
    VkCommandBuffer m_native_commands = VK_NULL_HANDLE;
};

} // namespace rhi::vulkan
