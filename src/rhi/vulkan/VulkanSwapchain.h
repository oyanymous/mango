#pragma once

#include <memory>
#include <vector>

#include "ISwapchain.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/**
 * The Vulkan ISwapchain. Created by VulkanDevice::createSwapchain.
 *
 * Images are presented from the device's graphics queue. The extent is clamped to what the surface allows. A
 * zero extent (a minimized window) is accepted: the swapchain then has no images and acquire() reports
 * AcquireStatus::OutOfDate until resize() is called with a non-zero size.
 */
class VulkanSwapchain final : public ISwapchain {
public:
    /**
     * Creates the swapchain.
     *
     * @return The swapchain; Unsupported when the device has no VK_KHR_swapchain, the graphics queue cannot
     *         present to the surface, or the format is not offered. An unavailable present mode falls back to
     *         PresentMode::Fifo.
     */
    static Result<SwapchainRef> create(std::shared_ptr<VulkanDeviceHandle> device, SwapchainDesc desc);

    /**
     * Stores the descriptor without creating a VkSwapchainKHR. Use create().
     *
     * @param desc The descriptor; its extent is replaced by the actual one when images are created.
     */
    VulkanSwapchain(std::shared_ptr<VulkanDeviceHandle> device, SwapchainDesc desc);
    /// Waits for the device to go idle, then destroys the swapchain.
    ~VulkanSwapchain() override;

    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    AcquireResult acquire(AcquireDesc desc) override;
    /// Waits for the device to go idle, then recreates the swapchain at @p extent (clamped to the surface).
    Result<void> resize(Extent2D extent) override;

    /// The VkSwapchainKHR; VK_NULL_HANDLE while the extent is zero.
    VkSwapchainKHR handle() const { return m_swapchain; }
private:
    // Builds (or rebuilds) the VkSwapchainKHR and backbuffers for m_desc.extent; replaces m_desc.extent with
    // the clamped size.
    Result<void> recreate();

    SwapchainDesc m_desc;
    std::vector<TextureRef> m_backbuffers;
    std::uint32_t m_current_image = 0;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
