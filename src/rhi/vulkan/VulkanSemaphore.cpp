/**
 * @file
 * @brief VulkanSemaphore.
 */

#include "VulkanSemaphore.h"

#include "VulkanDevice.h"

namespace rhi::vulkan {

Result<SemaphoreRef> VulkanSemaphore::create(std::shared_ptr<VulkanDeviceHandle> device, SemaphoreDesc desc) {
    const vk::SemaphoreTypeCreateInfo type_info{
        .semaphoreType = vk::SemaphoreType::eTimeline,
        .initialValue = desc.initial_value,
    };
    const vk::SemaphoreCreateInfo info{.pNext = desc.kind == SemaphoreKind::Timeline ? &type_info : nullptr};

    Result<vk::raii::Semaphore> semaphore = check(device->m_device.createSemaphore(info, nullptr), "vkCreateSemaphore");
    if (!semaphore) {
        return semaphore.error();
    }
    return SemaphoreRef(std::make_shared<VulkanSemaphore>(std::move(device), desc.kind, std::move(semaphore).value()));
}

VulkanSemaphore::VulkanSemaphore(std::shared_ptr<VulkanDeviceHandle> device, SemaphoreKind kind,
                                 vk::raii::Semaphore semaphore)
    : m_device(std::move(device)), m_kind(kind), m_semaphore(std::move(semaphore)) {
}

SemaphoreKind VulkanSemaphore::getKind() const {
    return m_kind;
}

std::uint64_t VulkanSemaphore::getValue() const {
    if (m_kind != SemaphoreKind::Timeline) {
        return 0;
    }
    auto value = m_semaphore.getCounterValue();
    return value.result == vk::Result::eSuccess ? value.value : 0;
}

Result<void> VulkanSemaphore::signal(std::uint64_t value) {
    if (m_kind != SemaphoreKind::Timeline) {
        return Error{ErrorCode::InvalidArgument, "ISemaphore::signal: only Timeline semaphores can be signaled"};
    }
    // Vulkan requires a strictly increasing value; checking here turns a validation error into a Result.
    if (value <= getValue()) {
        return Error{ErrorCode::InvalidArgument, "ISemaphore::signal: value must exceed the current value"};
    }
    return check(m_device->m_device.signalSemaphore({.semaphore = *m_semaphore, .value = value}), "vkSignalSemaphore");
}

} // namespace rhi::vulkan
