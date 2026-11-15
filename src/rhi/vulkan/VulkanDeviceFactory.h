#pragma once

#include "IDeviceFactory.h"

namespace rhi::vulkan {

class VulkanDeviceFactory final : public IDeviceFactory {
public:
    static bool isLoaderAvailable();

    Result<InstanceRef> createInstance(InstanceDesc desc) override;
private:
    [[maybe_unused]] Backend m_backend = Backend::Vulkan;
};

} // namespace rhi::vulkan
