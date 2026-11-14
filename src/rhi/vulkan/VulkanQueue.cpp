/// @file
/// VulkanQueue.

#include "VulkanQueue.h"

#include "VulkanDevice.h"

namespace rhi::vulkan {

VulkanQueue::VulkanQueue(std::shared_ptr<VulkanDeviceHandle> device, QueueType type, std::uint32_t family_index)
    : m_queue_type(type), m_family_index(family_index), m_device(std::move(device)) {
    vkGetDeviceQueue(m_device->m_device, family_index, 0, &m_queue);
}

Result<void> VulkanQueue::submit(SubmitDesc) {
    return Error{ErrorCode::Unsupported, "VulkanQueue::submit is implemented in step 5"};
}

Result<void> VulkanQueue::present(PresentDesc) {
    return Error{ErrorCode::Unsupported, "VulkanQueue::present is implemented in step 9"};
}

} // namespace rhi::vulkan
