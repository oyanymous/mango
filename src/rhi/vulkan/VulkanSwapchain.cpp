#include "VulkanSwapchain.h"

#include <algorithm>

#include "VulkanDevice.h"
#include "VulkanSemaphore.h"
#include "VulkanSurface.h"
#include "VulkanTexture.h"

namespace rhi::vulkan {

namespace {

vk::PresentModeKHR toVkPresentMode(PresentMode mode) {
    switch (mode) {
    case PresentMode::Fifo:
        return vk::PresentModeKHR::eFifo;
    case PresentMode::Mailbox:
        return vk::PresentModeKHR::eMailbox;
    case PresentMode::Immediate:
        return vk::PresentModeKHR::eImmediate;
    }
    return vk::PresentModeKHR::eFifo;
}

vk::ImageUsageFlags toVkSwapchainUsage(TextureUsage usage) {
    using Usage = vk::ImageUsageFlagBits;
    vk::ImageUsageFlags flags;
    if (hasFlag(usage, TextureUsage::RenderTarget)) {
        flags |= Usage::eColorAttachment;
    }
    if (hasFlag(usage, TextureUsage::CopyDst)) {
        flags |= Usage::eTransferDst;
    }
    if (hasFlag(usage, TextureUsage::CopySrc)) {
        flags |= Usage::eTransferSrc;
    }
    if (hasFlag(usage, TextureUsage::Sampled)) {
        flags |= Usage::eSampled;
    }
    if (hasFlag(usage, TextureUsage::Storage)) {
        flags |= Usage::eStorage;
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
    const vk::SurfaceKHR surface = static_cast<const VulkanSurface&>(*desc.surface).handle();

    const auto can_present = device->m_physical_device.getSurfaceSupportKHR(device->m_graphics_family, surface);
    if (can_present.result != vk::Result::eSuccess || !can_present.value) {
        return Error{ErrorCode::Unsupported, "createSwapchain: the graphics queue cannot present to this surface"};
    }

    auto swapchain = std::make_shared<VulkanSwapchain>(std::move(device), std::move(desc));
    if (Result<void> result = swapchain->recreate(); !result) {
        return result.error();
    }
    return SwapchainRef(std::move(swapchain));
}

VulkanSwapchain::VulkanSwapchain(std::shared_ptr<VulkanDeviceHandle> device, SwapchainDesc desc)
    : m_device(std::move(device)), m_desc(std::move(desc)) {
}

VulkanSwapchain::~VulkanSwapchain() {
    (void)m_device->m_device.waitIdle();
}

Result<void> VulkanSwapchain::recreate() {
    const vk::raii::PhysicalDevice& physical = m_device->m_physical_device;
    const vk::SurfaceKHR surface = static_cast<const VulkanSurface&>(*m_desc.surface).handle();

    Result<vk::SurfaceCapabilitiesKHR> caps_result =
        check(physical.getSurfaceCapabilitiesKHR(surface), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    if (!caps_result) {
        return caps_result.error();
    }
    const vk::SurfaceCapabilitiesKHR& caps = caps_result.value();

    Extent2D extent = m_desc.extent;
    if (caps.currentExtent.width != 0xFFFFFFFFu) {
        extent = {caps.currentExtent.width, caps.currentExtent.height};
    } else {
        extent.width = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    m_backbuffers.clear();
    m_desc.extent = extent;
    if (extent.width == 0 || extent.height == 0) {
        m_swapchain = nullptr;
        return {};
    }

    const std::vector<vk::SurfaceFormatKHR> formats = physical.getSurfaceFormatsKHR(surface).value;
    const vk::Format format = toVkFormat(m_desc.format);
    const bool format_ok = std::any_of(formats.begin(), formats.end(), [format](const vk::SurfaceFormatKHR& f) {
        return f.format == format && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    if (!format_ok) {
        m_swapchain = nullptr;
        return Error{ErrorCode::Unsupported, "createSwapchain: the surface does not offer this format"};
    }

    const std::vector<vk::PresentModeKHR> modes = physical.getSurfacePresentModesKHR(surface).value;
    vk::PresentModeKHR present_mode = toVkPresentMode(m_desc.present_mode);
    if (std::find(modes.begin(), modes.end(), present_mode) == modes.end()) {
        present_mode = vk::PresentModeKHR::eFifo;
    }

    std::uint32_t image_count = std::max(m_desc.image_count, caps.minImageCount);
    if (caps.maxImageCount != 0) {
        image_count = std::min(image_count, caps.maxImageCount);
    }

    vk::CompositeAlphaFlagBitsKHR alpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    if (!(caps.supportedCompositeAlpha & alpha)) {
        const auto bits = static_cast<VkCompositeAlphaFlagsKHR>(caps.supportedCompositeAlpha);
        alpha = static_cast<vk::CompositeAlphaFlagBitsKHR>(bits & (~bits + 1));
    }

    const vk::SwapchainCreateInfoKHR info{
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = format,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = {extent.width, extent.height},
        .imageArrayLayers = 1,
        .imageUsage = toVkSwapchainUsage(m_desc.usage),
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = caps.currentTransform,
        .compositeAlpha = alpha,
        .presentMode = present_mode,
        .clipped = vk::True,
        .oldSwapchain = *m_swapchain,
    };
    Result<vk::raii::SwapchainKHR> created =
        check(m_device->m_device.createSwapchainKHR(info, nullptr), "vkCreateSwapchainKHR");
    if (!created) {
        m_swapchain = nullptr;
        return created.error();
    }
    m_swapchain = std::move(created).value();

    Result<std::vector<vk::Image>> images = check(m_swapchain.getImages(), "vkGetSwapchainImagesKHR");
    if (!images) {
        m_swapchain = nullptr;
        return images.error();
    }

    const TextureDesc texture_desc{
        .format = m_desc.format,
        .extent = {extent.width, extent.height},
        .usage = m_desc.usage,
        .label = m_desc.label,
    };
    for (vk::Image image : images.value()) {
        m_backbuffers.push_back(std::make_shared<VulkanTexture>(m_device, texture_desc, image));
    }
    return {};
}

AcquireResult VulkanSwapchain::acquire(AcquireDesc desc) {
    if (!*m_swapchain) {
        return {.status = AcquireStatus::OutOfDate};
    }
    const vk::Semaphore semaphore =
        desc.signal_semaphore ? static_cast<const VulkanSemaphore&>(*desc.signal_semaphore).handle() : nullptr;
    const std::uint64_t timeout = desc.timeout == kWaitForever
                                      ? UINT64_MAX
                                      : static_cast<std::uint64_t>(std::max<Timeout::rep>(0, desc.timeout.count()));

    const vk::ResultValue<std::uint32_t> acquired = m_swapchain.acquireNextImage(timeout, semaphore, nullptr);
    switch (acquired.result) {
    case vk::Result::eSuccess:
    case vk::Result::eSuboptimalKHR:
        m_current_image = acquired.value;
        return {.status = acquired.result == vk::Result::eSuccess ? AcquireStatus::Success : AcquireStatus::Suboptimal,
                .image_index = m_current_image,
                .texture = m_backbuffers[m_current_image]};
    case vk::Result::eErrorOutOfDateKHR:
        return {.status = AcquireStatus::OutOfDate};
    case vk::Result::eTimeout:
    case vk::Result::eNotReady:
        return {.status = AcquireStatus::Timeout};
    default:
        return {.status = AcquireStatus::Error};
    }
}

Result<void> VulkanSwapchain::resize(Extent2D extent) {
    (void)m_device->m_device.waitIdle();
    m_desc.extent = extent;
    return recreate();
}

} // namespace rhi::vulkan
