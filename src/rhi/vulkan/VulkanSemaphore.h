#pragma once

#include <memory>

#include "ISemaphore.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

class VulkanSemaphore final : public ISemaphore {
public:
    static Result<SemaphoreRef> create(std::shared_ptr<VulkanDeviceHandle> device, SemaphoreDesc desc);

    VulkanSemaphore(std::shared_ptr<VulkanDeviceHandle> device, SemaphoreKind kind, vk::raii::Semaphore semaphore);

    VulkanSemaphore(const VulkanSemaphore&) = delete;
    VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;

    SemaphoreKind getKind() const override;

    std::uint64_t getValue() const override;

    Result<void> signal(std::uint64_t value) override;

    vk::Semaphore handle() const { return *m_semaphore; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    SemaphoreKind m_kind;
    vk::raii::Semaphore m_semaphore;
};

} // namespace rhi::vulkan
