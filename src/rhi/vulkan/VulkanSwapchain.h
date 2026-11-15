#pragma once

#include <memory>
#include <vector>

#include "ISwapchain.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

class VulkanSwapchain final : public ISwapchain {
public:
    static Result<SwapchainRef> create(std::shared_ptr<VulkanDeviceHandle> device, SwapchainDesc desc);

    VulkanSwapchain(std::shared_ptr<VulkanDeviceHandle> device, SwapchainDesc desc);

    ~VulkanSwapchain() override;

    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    AcquireResult acquire(AcquireDesc desc) override;

    Result<void> resize(Extent2D extent) override;

    vk::SwapchainKHR handle() const { return *m_swapchain; }
private:
    Result<void> recreate();

    std::shared_ptr<VulkanDeviceHandle> m_device;
    SwapchainDesc m_desc;
    vk::raii::SwapchainKHR m_swapchain = nullptr;
    std::vector<TextureRef> m_backbuffers;
    std::uint32_t m_current_image = 0;
};

} // namespace rhi::vulkan
