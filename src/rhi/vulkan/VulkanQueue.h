#pragma once

/**
 * @file
 * @brief rhi::vulkan::VulkanQueue: a VkQueue of one queue family.
 */

#include <cstdint>
#include <memory>

#include "IQueue.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/**
 * @brief The Vulkan IQueue. Created by VulkanAdapter::createDevice, one per used queue family.
 */
class VulkanQueue final : public IQueue {
public:
    /**
     * @brief Wraps queue 0 of a queue family.
     *
     * @param device Owner of the VkDevice; kept alive by the queue.
     * @param type The QueueType the queue was created for.
     * @param family_index Queue family the queue belongs to.
     */
    VulkanQueue(std::shared_ptr<VulkanDeviceHandle> device, QueueType type, std::uint32_t family_index);

    /**
     * @brief Submits with vkQueueSubmit; every command buffer must be ended (else InvalidArgument).
     *
     * @details
     * Does not yet retain the command buffers until they finish (step 13 of docs/RHI_VULKAN_PLAN.md): keep
     * them alive until the submission's fence is signaled.
     */
    Result<void> submit(SubmitDesc desc) override;
    /**
     * @brief Presents with vkQueuePresentKHR.
     *
     * @return Success, or ErrorCode::OutOfDate when the swapchain is out of date, suboptimal (the image was still
     *         presented) or has a zero extent.
     */
    Result<void> present(PresentDesc desc) override;

    /**
     * @brief The QueueType the queue was created for.
     */
    QueueType type() const { return m_queue_type; }
    /**
     * @brief Queue family index, needed for command pools and ownership transfers.
     */
    std::uint32_t familyIndex() const { return m_family_index; }
private:
    QueueType m_queue_type;
    std::uint32_t m_family_index;
    VkQueue m_queue = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
