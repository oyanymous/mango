#pragma once

#include <memory>

#include "ISemaphore.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/// The Vulkan ISemaphore. Created by VulkanDevice::createSemaphore.
class VulkanSemaphore final : public ISemaphore {
public:
    static Result<SemaphoreRef> create(std::shared_ptr<VulkanDeviceHandle> device, SemaphoreDesc desc);

    /// Takes ownership of a created VkSemaphore. Use create().
    VulkanSemaphore(std::shared_ptr<VulkanDeviceHandle> device, SemaphoreKind kind, VkSemaphore semaphore);
    ~VulkanSemaphore() override;

    VulkanSemaphore(const VulkanSemaphore&) = delete;
    VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;

    SemaphoreKind getKind() const override;
    /// Reads the counter with vkGetSemaphoreCounterValue (Timeline only).
    std::uint64_t getValue() const override;
    /// Signals with vkSignalSemaphore (Timeline only).
    Result<void> signal(std::uint64_t value) override;

    /// The VkSemaphore, for VkQueue submissions.
    VkSemaphore handle() const { return m_semaphore; }
private:
    SemaphoreKind m_kind;
    VkSemaphore m_semaphore = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
