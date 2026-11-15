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

    /// Takes ownership of a created semaphore. Use create().
    VulkanSemaphore(std::shared_ptr<VulkanDeviceHandle> device, SemaphoreKind kind, vk::raii::Semaphore semaphore);

    VulkanSemaphore(const VulkanSemaphore&) = delete;
    VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;

    SemaphoreKind getKind() const override;
    /// Reads the counter with vkGetSemaphoreCounterValue (Timeline only; 0 for Binary or on device loss).
    std::uint64_t getValue() const override;
    /// Signals with vkSignalSemaphore (Timeline only).
    Result<void> signal(std::uint64_t value) override;

    /// The VkSemaphore, for VkQueue submissions.
    VkSemaphore handle() const { return static_cast<VkSemaphore>(*m_semaphore); }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device; // first: destroyed after the semaphore
    SemaphoreKind m_kind;
    vk::raii::Semaphore m_semaphore;
};

} // namespace rhi::vulkan
