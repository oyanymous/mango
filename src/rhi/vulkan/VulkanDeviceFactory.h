#pragma once

/**
 * @file
 * @brief rhi::vulkan::VulkanDeviceFactory: the Vulkan backend's entry point.
 */

#include "IDeviceFactory.h"

namespace rhi::vulkan {

/**
 * @brief Creates Vulkan instances. Registered in RHIFactory for Backend::Vulkan.
 *
 * @details
 * createInstance() loads the Vulkan loader through volk, enables the validation layer and debug messenger
 * when requested, and enables the surface extensions the platform needs (plus portability enumeration
 * for MoltenVK).
 */
class VulkanDeviceFactory final : public IDeviceFactory {
public:
    /**
     * @brief True if the Vulkan loader that ships with the GPU driver can be loaded.
     */
    static bool isLoaderAvailable();

    Result<InstanceRef> createInstance(InstanceDesc desc) override;
private:
    [[maybe_unused]] Backend m_backend = Backend::Vulkan;
};

} // namespace rhi::vulkan
