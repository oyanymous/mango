#pragma once

/**
 * @file
 * @brief rhi::vulkan::VulkanFence: a VkFence.
 */

#include <memory>

#include "IFence.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/**
 * @brief The Vulkan IFence. Created by VulkanDevice::createFence.
 */
class VulkanFence final : public IFence {
public:
    /**
     * @brief Creates the VkFence.
     *
     * @param device Owner of the VkDevice; kept alive by the fence.
     * @param desc Initial state.
     */
    static Result<FenceRef> create(std::shared_ptr<VulkanDeviceHandle> device, FenceDesc desc);

    /**
     * @brief Takes ownership of a created VkFence. Use create().
     *
     * @param device Owner of the VkDevice.
     * @param fence The fence.
     */
    VulkanFence(std::shared_ptr<VulkanDeviceHandle> device, VkFence fence);
    /**
     * @brief Destroys the VkFence.
     */
    ~VulkanFence() override;

    VulkanFence(const VulkanFence&) = delete;
    VulkanFence& operator=(const VulkanFence&) = delete;

    Result<void> wait(Timeout timeout) override;
    bool isSignaled() const override;
    Result<void> reset() override;

    /**
     * @brief The VkFence, for VkQueue submissions.
     */
    VkFence handle() const { return m_fence; }
private:
    VkFence m_fence = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
