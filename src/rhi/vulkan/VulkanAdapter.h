#pragma once

#include <memory>
#include <optional>

#include "IAdapter.h"
#include "VulkanCommon.h"
#include "VulkanInstance.h"

namespace rhi::vulkan {

class VulkanAdapter final : public IAdapter {
public:
    static std::optional<AdapterRef> create(std::shared_ptr<VulkanInstanceHandle> instance,
                                            const vk::raii::PhysicalDevice& physical_device);

    VulkanAdapter(std::shared_ptr<VulkanInstanceHandle> instance, vk::raii::PhysicalDevice physical_device,
                  Capabilities capabilities);

    Capabilities getCapabilities() const override;

    Result<DeviceRef> createDevice(DeviceDesc desc) override;
private:
    std::shared_ptr<VulkanInstanceHandle> m_instance;
    Capabilities m_capabilities;
    vk::raii::PhysicalDevice m_physical_device;
};

} // namespace rhi::vulkan
