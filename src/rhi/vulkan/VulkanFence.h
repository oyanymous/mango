#pragma once

#include <memory>

#include "IFence.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/// The Vulkan IFence. Created by VulkanDevice::createFence.
class VulkanFence final : public IFence {
public:
    static Result<FenceRef> create(std::shared_ptr<VulkanDeviceHandle> device, FenceDesc desc);

    /// Takes ownership of a created VkFence. Use create().
    VulkanFence(std::shared_ptr<VulkanDeviceHandle> device, VkFence fence);
    ~VulkanFence() override;

    VulkanFence(const VulkanFence&) = delete;
    VulkanFence& operator=(const VulkanFence&) = delete;

    Result<void> wait(Timeout timeout) override;
    bool isSignaled() const override;
    Result<void> reset() override;

    /// The VkFence, for VkQueue submissions.
    VkFence handle() const { return m_fence; }
private:
    VkFence m_fence = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
