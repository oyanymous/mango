#pragma once

#include "IDeviceFactory.h"

namespace rhi::vulkan {

/**
 * Creates Vulkan instances. Registered in RHIFactory for Backend::Vulkan.
 *
 * createInstance() loads the Vulkan loader through volk, enables the validation layer and debug messenger
 * when requested, and enables the surface extensions the platform needs (plus portability enumeration
 * for MoltenVK).
 */
class VulkanDeviceFactory final : public IDeviceFactory {
public:
    /// True if the Vulkan loader that ships with the GPU driver can be loaded.
    static bool isLoaderAvailable();

    Result<InstanceRef> createInstance(InstanceDesc desc) override;
private:
    [[maybe_unused]] Backend m_backend = Backend::Vulkan;
};

} // namespace rhi::vulkan
