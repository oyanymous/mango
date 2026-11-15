/**
 * @file
 * @brief VulkanSurface: platform surface creation.
 */

#include "VulkanSurface.h"

namespace rhi::vulkan {

Result<SurfaceRef> VulkanSurface::create(std::shared_ptr<VulkanInstanceHandle> instance, SurfaceDesc desc) {
    const vk::raii::Instance& vk_instance = instance->m_instance;
    // A create function is null when the instance was created without its extension.
    vk::ResultValue<vk::raii::SurfaceKHR> surface{vk::Result::eErrorExtensionNotPresent, nullptr};

    if (const auto* win32 = std::get_if<Win32WindowHandle>(&desc.window)) {
        if (!win32->hwnd) {
            return Error{ErrorCode::InvalidArgument, "createSurface: hwnd is null"};
        }
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        if (vk_instance.getDispatcher()->vkCreateWin32SurfaceKHR) {
            surface = vk_instance.createWin32SurfaceKHR(vk::Win32SurfaceCreateInfoKHR{
                .hinstance = static_cast<HINSTANCE>(win32->hinstance),
                .hwnd = static_cast<HWND>(win32->hwnd),
            });
        }
#endif
    } else if (const auto* metal = std::get_if<MetalLayerHandle>(&desc.window)) {
        if (!metal->layer) {
            return Error{ErrorCode::InvalidArgument, "createSurface: layer is null"};
        }
#if defined(VK_USE_PLATFORM_METAL_EXT)
        // shortcut: written but never run; verify on the first macOS build with MoltenVK.
        if (vk_instance.getDispatcher()->vkCreateMetalSurfaceEXT) {
            surface = vk_instance.createMetalSurfaceEXT(vk::MetalSurfaceCreateInfoEXT{
                .pLayer = static_cast<const CAMetalLayer*>(metal->layer),
            });
        }
#endif
    } else {
        return Error{ErrorCode::InvalidArgument, "createSurface: no window handle"};
    }

    if (surface.result == vk::Result::eErrorExtensionNotPresent) {
        return Error{ErrorCode::Unsupported, "createSurface: this platform's surface extension is not available"};
    }
    Result<vk::raii::SurfaceKHR> created = check(std::move(surface), "vkCreate*SurfaceKHR");
    if (!created) {
        return created.error();
    }
    return SurfaceRef(
        std::make_shared<VulkanSurface>(std::move(instance), std::move(desc), std::move(created).value()));
}

VulkanSurface::VulkanSurface(std::shared_ptr<VulkanInstanceHandle> instance, SurfaceDesc desc,
                             vk::raii::SurfaceKHR surface)
    : m_instance(std::move(instance)), m_desc(std::move(desc)), m_surface(std::move(surface)) {
}

SurfaceDesc VulkanSurface::getDesc() const {
    return m_desc;
}

} // namespace rhi::vulkan
