#pragma once

#include <memory>

#include "IFence.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

class VulkanFence final : public IFence {
public:
    static Result<FenceRef> create(std::shared_ptr<VulkanDeviceHandle> device, FenceDesc desc);

    VulkanFence(std::shared_ptr<VulkanDeviceHandle> device, vk::raii::Fence fence);

    VulkanFence(const VulkanFence&) = delete;
    VulkanFence& operator=(const VulkanFence&) = delete;

    Result<void> wait(Timeout timeout) override;
    bool isSignaled() const override;
    Result<void> reset() override;

    vk::Fence handle() const { return *m_fence; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    vk::raii::Fence m_fence;
};

} // namespace rhi::vulkan
