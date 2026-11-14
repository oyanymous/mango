#pragma once

/**
 * @file
 * @brief rhi::vulkan::VulkanSurface: a VkSurfaceKHR for a native window.
 */

#include <memory>

#include "ISurface.h"
#include "Result.h"
#include "VulkanCommon.h"
#include "VulkanInstance.h"

namespace rhi::vulkan {

/**
 * @brief The Vulkan ISurface. Created by VulkanInstance::createSurface.
 *
 * @details
 * Keeps the instance alive, so it may outlive the InstanceRef. The window must outlive the surface.
 */
class VulkanSurface final : public ISurface {
public:
    /**
     * @brief Creates a surface for a Win32 window or a CAMetalLayer.
     *
     * @param instance Owner of the VkInstance; kept alive by the surface.
     * @param desc The native window.
     * @return The surface; InvalidArgument for an empty handle, or Unsupported when the instance lacks the
     *         platform's surface extension.
     */
    static Result<SurfaceRef> create(std::shared_ptr<VulkanInstanceHandle> instance, SurfaceDesc desc);

    /**
     * @brief Takes ownership of a created surface. Use create().
     *
     * @param instance Owner of the VkInstance.
     * @param desc The descriptor it was created with.
     * @param surface The surface.
     */
    VulkanSurface(std::shared_ptr<VulkanInstanceHandle> instance, SurfaceDesc desc, VkSurfaceKHR surface);
    /**
     * @brief Destroys the surface.
     */
    ~VulkanSurface() override;

    VulkanSurface(const VulkanSurface&) = delete;
    VulkanSurface& operator=(const VulkanSurface&) = delete;

    SurfaceDesc getDesc() const override;

    /**
     * @brief The VkSurfaceKHR, for swapchains.
     */
    VkSurfaceKHR handle() const { return m_surface; }
private:
    SurfaceDesc m_desc;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    std::shared_ptr<VulkanInstanceHandle> m_instance;
};

} // namespace rhi::vulkan
