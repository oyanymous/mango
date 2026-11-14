/**
 * @file
 * @brief VulkanFence.
 */

#include "VulkanFence.h"

#include <algorithm>
#include <cstdint>
#include <limits>

#include "VulkanDevice.h"

namespace rhi::vulkan {

Result<FenceRef> VulkanFence::create(std::shared_ptr<VulkanDeviceHandle> device, FenceDesc desc) {
    VkFenceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = desc.signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    VkFence fence = VK_NULL_HANDLE;
    if (VkResult result = vkCreateFence(device->m_device, &info, nullptr, &fence); result != VK_SUCCESS) {
        return toError(result, "vkCreateFence");
    }
    return FenceRef(std::make_shared<VulkanFence>(std::move(device), fence));
}

VulkanFence::VulkanFence(std::shared_ptr<VulkanDeviceHandle> device, VkFence fence)
    : m_fence(fence), m_device(std::move(device)) {
}

VulkanFence::~VulkanFence() {
    vkDestroyFence(m_device->m_device, m_fence, nullptr);
}

Result<void> VulkanFence::wait(Timeout timeout) {
    // Vulkan takes unsigned nanoseconds, with UINT64_MAX meaning forever; negative timeouts poll.
    const std::uint64_t ns = timeout == kWaitForever
                                 ? std::numeric_limits<std::uint64_t>::max()
                                 : static_cast<std::uint64_t>(std::max<Timeout::rep>(0, timeout.count()));
    if (VkResult result = vkWaitForFences(m_device->m_device, 1, &m_fence, VK_TRUE, ns); result != VK_SUCCESS) {
        return toError(result, "vkWaitForFences");
    }
    return {};
}

bool VulkanFence::isSignaled() const {
    return vkGetFenceStatus(m_device->m_device, m_fence) == VK_SUCCESS;
}

Result<void> VulkanFence::reset() {
    if (VkResult result = vkResetFences(m_device->m_device, 1, &m_fence); result != VK_SUCCESS) {
        return toError(result, "vkResetFences");
    }
    return {};
}

} // namespace rhi::vulkan
