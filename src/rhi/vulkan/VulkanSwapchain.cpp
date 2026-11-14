/**
 * @file
 * @brief VulkanSwapchain.
 */

#include "VulkanSwapchain.h"

#include <algorithm>

#include "VulkanDevice.h"
#include "VulkanSemaphore.h"
#include "VulkanSurface.h"
#include "VulkanTexture.h"

namespace rhi::vulkan {

namespace {

/**
 * @brief Maps a PresentMode to its VkPresentModeKHR.
 */
VkPresentModeKHR toVkPresentMode(PresentMode mode) {
    switch (mode) {
    case PresentMode::Fifo:
        return VK_PRESENT_MODE_FIFO_KHR;
    case PresentMode::Mailbox:
        return VK_PRESENT_MODE_MAILBOX_KHR;
    case PresentMode::Immediate:
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

/**
 * @brief Maps the TextureUsage bits a backbuffer can have to VkImageUsageFlags.
 */
VkImageUsageFlags toVkSwapchainUsage(TextureUsage usage) {
    VkImageUsageFlags flags = 0;
    if (hasFlag(usage, TextureUsage::RenderTarget)) {
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (hasFlag(usage, TextureUsage::CopyDst)) {
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (hasFlag(usage, TextureUsage::CopySrc)) {
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (hasFlag(usage, TextureUsage::Sampled)) {
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (hasFlag(usage, TextureUsage::Storage)) {
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    return flags;
}

} // namespace

Result<SwapchainRef> VulkanSwapchain::create(std::shared_ptr<VulkanDeviceHandle> device, SwapchainDesc desc) {
    if (!device->m_swapchain_enabled) {
        return Error{ErrorCode::Unsupported, "createSwapchain: the device has no VK_KHR_swapchain"};
    }
    if (!desc.surface) {
        return Error{ErrorCode::InvalidArgument, "createSwapchain: surface is null"};
    }
    const VkSurfaceKHR surface = static_cast<const VulkanSurface&>(*desc.surface).handle();

    VkBool32 can_present = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(device->m_physical_device, device->m_graphics_family, surface, &can_present);
    if (!can_present) {
        return Error{ErrorCode::Unsupported, "createSwapchain: the graphics queue cannot present to this surface"};
    }

    auto swapchain = std::make_shared<VulkanSwapchain>(std::move(device), std::move(desc));
    if (Result<void> result = swapchain->recreate(); !result) {
        return result.error();
    }
    return SwapchainRef(std::move(swapchain));
}

VulkanSwapchain::VulkanSwapchain(std::shared_ptr<VulkanDeviceHandle> device, SwapchainDesc desc)
    : m_desc(std::move(desc)), m_device(std::move(device)) {
}

VulkanSwapchain::~VulkanSwapchain() {
    // Presentation may still read the images; the backbuffer textures do not own them.
    vkDeviceWaitIdle(m_device->m_device);
    m_backbuffers.clear();
    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device->m_device, m_swapchain, nullptr);
    }
}

Result<void> VulkanSwapchain::recreate() {
    const VkPhysicalDevice physical = m_device->m_physical_device;
    const VkSurfaceKHR surface = static_cast<const VulkanSurface&>(*m_desc.surface).handle();

    VkSurfaceCapabilitiesKHR caps{};
    if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps); result != VK_SUCCESS) {
        return toError(result, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    }

    // Windows reports the window's size as currentExtent; 0xFFFFFFFF means "whatever the swapchain says".
    Extent2D extent = m_desc.extent;
    if (caps.currentExtent.width != 0xFFFFFFFFu) {
        extent = {caps.currentExtent.width, caps.currentExtent.height};
    } else {
        extent.width = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    const VkSwapchainKHR old_swapchain = m_swapchain;
    m_backbuffers.clear();
    m_swapchain = VK_NULL_HANDLE;
    m_desc.extent = extent;
    if (extent.width == 0 || extent.height == 0) {
        // Minimized: no images until the next non-zero resize.
        if (old_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device->m_device, old_swapchain, nullptr);
        }
        return {};
    }

    std::uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &format_count, formats.data());
    const VkFormat format = toVkFormat(m_desc.format);
    const bool format_ok = std::any_of(formats.begin(), formats.end(), [format](const VkSurfaceFormatKHR& f) {
        return f.format == format && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });
    if (!format_ok) {
        if (old_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device->m_device, old_swapchain, nullptr);
        }
        return Error{ErrorCode::Unsupported, "createSwapchain: the surface does not offer this format"};
    }

    std::uint32_t mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &mode_count, nullptr);
    std::vector<VkPresentModeKHR> modes(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &mode_count, modes.data());
    VkPresentModeKHR present_mode = toVkPresentMode(m_desc.present_mode);
    if (std::find(modes.begin(), modes.end(), present_mode) == modes.end()) {
        present_mode = VK_PRESENT_MODE_FIFO_KHR; // the only mode every implementation supports
    }

    std::uint32_t image_count = std::max(m_desc.image_count, caps.minImageCount);
    if (caps.maxImageCount != 0) {
        image_count = std::min(image_count, caps.maxImageCount);
    }

    VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (!(caps.supportedCompositeAlpha & alpha)) {
        // Take any supported mode; the lowest set bit is a valid single flag.
        alpha = static_cast<VkCompositeAlphaFlagBitsKHR>(caps.supportedCompositeAlpha &
                                                         (~caps.supportedCompositeAlpha + 1));
    }

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface;
    info.minImageCount = image_count;
    info.imageFormat = format;
    info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    info.imageExtent = {extent.width, extent.height};
    info.imageArrayLayers = 1;
    info.imageUsage = toVkSwapchainUsage(m_desc.usage);
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = alpha;
    info.presentMode = present_mode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = old_swapchain;

    const VkResult result = vkCreateSwapchainKHR(m_device->m_device, &info, nullptr, &m_swapchain);
    if (old_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device->m_device, old_swapchain, nullptr);
    }
    if (result != VK_SUCCESS) {
        m_swapchain = VK_NULL_HANDLE;
        return toError(result, "vkCreateSwapchainKHR");
    }

    std::uint32_t count = 0;
    vkGetSwapchainImagesKHR(m_device->m_device, m_swapchain, &count, nullptr);
    std::vector<VkImage> images(count);
    vkGetSwapchainImagesKHR(m_device->m_device, m_swapchain, &count, images.data());

    const TextureDesc texture_desc{
        .format = m_desc.format,
        .extent = {extent.width, extent.height},
        .usage = m_desc.usage,
        .label = m_desc.label,
    };
    for (VkImage image : images) {
        m_backbuffers.push_back(std::make_shared<VulkanTexture>(m_device, texture_desc, image, VK_NULL_HANDLE));
    }
    return {};
}

AcquireResult VulkanSwapchain::acquire(AcquireDesc desc) {
    if (m_swapchain == VK_NULL_HANDLE) {
        return {.status = AcquireStatus::OutOfDate};
    }
    const VkSemaphore semaphore =
        desc.signal_semaphore ? static_cast<const VulkanSemaphore&>(*desc.signal_semaphore).handle() : VK_NULL_HANDLE;
    const std::uint64_t timeout = desc.timeout == kWaitForever
                                      ? UINT64_MAX
                                      : static_cast<std::uint64_t>(std::max<Timeout::rep>(0, desc.timeout.count()));

    const VkResult result =
        vkAcquireNextImageKHR(m_device->m_device, m_swapchain, timeout, semaphore, VK_NULL_HANDLE, &m_current_image);
    switch (result) {
    case VK_SUCCESS:
        return {.status = AcquireStatus::Success,
                .image_index = m_current_image,
                .texture = m_backbuffers[m_current_image]};
    case VK_SUBOPTIMAL_KHR:
        return {.status = AcquireStatus::Suboptimal,
                .image_index = m_current_image,
                .texture = m_backbuffers[m_current_image]};
    case VK_ERROR_OUT_OF_DATE_KHR:
        return {.status = AcquireStatus::OutOfDate};
    case VK_TIMEOUT:
    case VK_NOT_READY:
        return {.status = AcquireStatus::Timeout};
    default:
        return {.status = AcquireStatus::Error};
    }
}

Result<void> VulkanSwapchain::resize(Extent2D extent) {
    // The old images may still be in flight or queued for presentation.
    vkDeviceWaitIdle(m_device->m_device);
    m_desc.extent = extent;
    return recreate();
}

} // namespace rhi::vulkan
