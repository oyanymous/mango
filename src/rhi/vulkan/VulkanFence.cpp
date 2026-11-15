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
    const vk::FenceCreateInfo info{.flags =
                                       desc.signaled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlags{}};
    Result<vk::raii::Fence> fence = check(device->m_device.createFence(info, nullptr), "vkCreateFence");
    if (!fence) {
        return fence.error();
    }
    return FenceRef(std::make_shared<VulkanFence>(std::move(device), std::move(fence).value()));
}

VulkanFence::VulkanFence(std::shared_ptr<VulkanDeviceHandle> device, vk::raii::Fence fence)
    : m_device(std::move(device)), m_fence(std::move(fence)) {
}

Result<void> VulkanFence::wait(Timeout timeout) {
    // Vulkan takes unsigned nanoseconds, with UINT64_MAX meaning forever; negative timeouts poll.
    const std::uint64_t ns = timeout == kWaitForever
                                 ? std::numeric_limits<std::uint64_t>::max()
                                 : static_cast<std::uint64_t>(std::max<Timeout::rep>(0, timeout.count()));
    return check(m_device->m_device.waitForFences(*m_fence, vk::True, ns), "vkWaitForFences");
}

bool VulkanFence::isSignaled() const {
    return m_fence.getStatus() == vk::Result::eSuccess;
}

Result<void> VulkanFence::reset() {
    return check(m_device->m_device.resetFences(*m_fence), "vkResetFences");
}

} // namespace rhi::vulkan
