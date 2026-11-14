#pragma once

/// @file
/// rhi::vulkan::VulkanAdapter: a Vulkan physical device.

#include <memory>
#include <optional>

#include "IAdapter.h"
#include "VulkanCommon.h"
#include "VulkanInstance.h"

namespace rhi::vulkan {

/// The Vulkan IAdapter: wraps a VkPhysicalDevice and its queried Capabilities.
class VulkanAdapter final : public IAdapter {
public:
    /// Queries a physical device and wraps it if the backend can use it.
    /// @param instance Owner of the VkInstance; kept alive by the adapter.
    /// @param physical_device Device from vkEnumeratePhysicalDevices.
    /// @return The adapter, or nothing for GPUs below Vulkan 1.2 or without timeline semaphores.
    static std::optional<AdapterRef> create(std::shared_ptr<VulkanInstanceHandle> instance,
                                            VkPhysicalDevice physical_device);

    /// Wraps a physical device whose capabilities were already queried. Use create().
    /// @param instance Owner of the VkInstance.
    /// @param physical_device The physical device.
    /// @param capabilities Its capabilities.
    VulkanAdapter(std::shared_ptr<VulkanInstanceHandle> instance, VkPhysicalDevice physical_device,
                  Capabilities capabilities);

    Capabilities getCapabilities() const override;

    /// Creates a VulkanDevice: one graphics queue plus dedicated compute/transfer queues where the adapter has
    /// such families, timeline semaphores, VK_KHR_swapchain (if available) and a VMA allocator.
    Result<DeviceRef> createDevice(DeviceDesc desc) override;
private:
    Capabilities m_capabilities;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    std::shared_ptr<VulkanInstanceHandle> m_instance;
};

} // namespace rhi::vulkan
