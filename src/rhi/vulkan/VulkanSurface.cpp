/**
 * @file
 * @brief VulkanSurface: platform surface creation.
 */

#include "VulkanSurface.h"

namespace rhi::vulkan {

Result<SurfaceRef> VulkanSurface::create(std::shared_ptr<VulkanInstanceHandle> instance, SurfaceDesc desc) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;

    if (const auto* win32 = std::get_if<Win32WindowHandle>(&desc.window)) {
        if (!win32->hwnd) {
            return Error{ErrorCode::InvalidArgument, "createSurface: hwnd is null"};
        }
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        // volk leaves the pointer null when the instance was created without VK_KHR_win32_surface.
        if (vkCreateWin32SurfaceKHR) {
            VkWin32SurfaceCreateInfoKHR info{};
            info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            info.hinstance = static_cast<HINSTANCE>(win32->hinstance);
            info.hwnd = static_cast<HWND>(win32->hwnd);
            result = vkCreateWin32SurfaceKHR(instance->get(), &info, nullptr, &surface);
        }
#endif
    } else if (const auto* metal = std::get_if<MetalLayerHandle>(&desc.window)) {
        if (!metal->layer) {
            return Error{ErrorCode::InvalidArgument, "createSurface: layer is null"};
        }
#if defined(VK_USE_PLATFORM_METAL_EXT)
        // shortcut: written but never run; verify on the first macOS build with MoltenVK.
        if (vkCreateMetalSurfaceEXT) {
            VkMetalSurfaceCreateInfoEXT info{};
            info.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
            info.pLayer = static_cast<const CAMetalLayer*>(metal->layer);
            result = vkCreateMetalSurfaceEXT(instance->get(), &info, nullptr, &surface);
        }
#endif
    } else {
        return Error{ErrorCode::InvalidArgument, "createSurface: no window handle"};
    }

    if (result == VK_ERROR_EXTENSION_NOT_PRESENT) {
        return Error{ErrorCode::Unsupported, "createSurface: this platform's surface extension is not available"};
    }
    if (result != VK_SUCCESS) {
        return toError(result, "vkCreate*SurfaceKHR");
    }
    return SurfaceRef(std::make_shared<VulkanSurface>(std::move(instance), std::move(desc), surface));
}

VulkanSurface::VulkanSurface(std::shared_ptr<VulkanInstanceHandle> instance, SurfaceDesc desc, VkSurfaceKHR surface)
    : m_desc(std::move(desc)), m_surface(surface), m_instance(std::move(instance)) {
}

VulkanSurface::~VulkanSurface() {
    vkDestroySurfaceKHR(m_instance->get(), m_surface, nullptr);
}

SurfaceDesc VulkanSurface::getDesc() const {
    return m_desc;
}

} // namespace rhi::vulkan
