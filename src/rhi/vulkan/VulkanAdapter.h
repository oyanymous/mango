#pragma once

#include <memory>
#include <optional>

#include "IAdapter.h"
#include "VulkanCommon.h"
#include "VulkanInstance.h"

namespace rhi::vulkan {

/// The Vulkan IAdapter: wraps a physical device and its queried Capabilities.
class VulkanAdapter final : public IAdapter {
public:
    /**
     * Queries a physical device and wraps it if the backend can use it.
     *
     * @param physical_device From the instance's enumeratePhysicalDevices().
     * @return The adapter, or nothing for GPUs below Vulkan 1.2 or without timeline semaphores.
     */
    static std::optional<AdapterRef> create(std::shared_ptr<VulkanInstanceHandle> instance,
                                            const vk::raii::PhysicalDevice& physical_device);

    /// Wraps a physical device whose capabilities were already queried. Use create().
    VulkanAdapter(std::shared_ptr<VulkanInstanceHandle> instance, vk::raii::PhysicalDevice physical_device,
                  Capabilities capabilities);

    Capabilities getCapabilities() const override;

    /// Creates a VulkanDevice: one graphics queue plus dedicated compute/transfer queues where the adapter has such
    /// families, timeline semaphores, VK_KHR_swapchain (if available) and a VMA allocator.
    Result<DeviceRef> createDevice(DeviceDesc desc) override;
private:
    std::shared_ptr<VulkanInstanceHandle> m_instance; // first: m_physical_device uses its dispatch table
    Capabilities m_capabilities;
    vk::raii::PhysicalDevice m_physical_device;
};

} // namespace rhi::vulkan
