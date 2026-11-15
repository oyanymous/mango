#pragma once

#include <memory>

#include "ISurface.h"
#include "Result.h"
#include "VulkanCommon.h"
#include "VulkanInstance.h"

namespace rhi::vulkan {

/**
 * The Vulkan ISurface. Created by VulkanInstance::createSurface.
 *
 * Keeps the instance alive, so it may outlive the InstanceRef. The window must outlive the surface.
 */
class VulkanSurface final : public ISurface {
public:
    /**
     * Creates a surface for a Win32 window or a CAMetalLayer.
     *
     * @return The surface; InvalidArgument for an empty handle, or Unsupported when the instance lacks the
     *         platform's surface extension.
     */
    static Result<SurfaceRef> create(std::shared_ptr<VulkanInstanceHandle> instance, SurfaceDesc desc);

    /// Takes ownership of a created surface. Use create().
    VulkanSurface(std::shared_ptr<VulkanInstanceHandle> instance, SurfaceDesc desc, VkSurfaceKHR surface);
    ~VulkanSurface() override;

    VulkanSurface(const VulkanSurface&) = delete;
    VulkanSurface& operator=(const VulkanSurface&) = delete;

    SurfaceDesc getDesc() const override;

    /// The VkSurfaceKHR, for swapchains.
    VkSurfaceKHR handle() const { return m_surface; }
private:
    SurfaceDesc m_desc;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    std::shared_ptr<VulkanInstanceHandle> m_instance;
};

} // namespace rhi::vulkan
