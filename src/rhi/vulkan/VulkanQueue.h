#pragma once

/// @file
/// rhi::vulkan::VulkanQueue: a VkQueue of one queue family.

#include <cstdint>
#include <memory>

#include "IQueue.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/// The Vulkan IQueue. Created by VulkanAdapter::createDevice, one per used queue family.
class VulkanQueue final : public IQueue {
public:
    /// Wraps queue 0 of a queue family.
    /// @param device Owner of the VkDevice; kept alive by the queue.
    /// @param type The QueueType the queue was created for.
    /// @param family_index Queue family the queue belongs to.
    VulkanQueue(std::shared_ptr<VulkanDeviceHandle> device, QueueType type, std::uint32_t family_index);

    /// Not implemented yet (step 5 of docs/RHI_VULKAN_PLAN.md); returns ErrorCode::Unsupported.
    Result<void> submit(SubmitDesc desc) override;
    /// Not implemented yet (step 9 of docs/RHI_VULKAN_PLAN.md); returns ErrorCode::Unsupported.
    Result<void> present(PresentDesc desc) override;

    /// The QueueType the queue was created for.
    QueueType type() const { return m_queue_type; }
    /// Queue family index, needed for command pools and ownership transfers.
    std::uint32_t familyIndex() const { return m_family_index; }
private:
    QueueType m_queue_type;
    std::uint32_t m_family_index;
    VkQueue m_queue = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
