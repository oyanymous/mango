/**
 * @file
 * @brief VulkanSemaphore.
 */

#include "VulkanSemaphore.h"

#include "VulkanDevice.h"

namespace rhi::vulkan {

Result<SemaphoreRef> VulkanSemaphore::create(std::shared_ptr<VulkanDeviceHandle> device, SemaphoreDesc desc) {
    VkSemaphoreTypeCreateInfo type_info{};
    type_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    type_info.initialValue = desc.initial_value;

    VkSemaphoreCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = desc.kind == SemaphoreKind::Timeline ? &type_info : nullptr;

    VkSemaphore semaphore = VK_NULL_HANDLE;
    if (VkResult result = vkCreateSemaphore(device->vkDevice(), &info, nullptr, &semaphore); result != VK_SUCCESS) {
        return toError(result, "vkCreateSemaphore");
    }
    return SemaphoreRef(std::make_shared<VulkanSemaphore>(std::move(device), desc.kind, semaphore));
}

VulkanSemaphore::VulkanSemaphore(std::shared_ptr<VulkanDeviceHandle> device, SemaphoreKind kind, VkSemaphore semaphore)
    : m_kind(kind), m_semaphore(semaphore), m_device(std::move(device)) {
}

VulkanSemaphore::~VulkanSemaphore() {
    vkDestroySemaphore(m_device->vkDevice(), m_semaphore, nullptr);
}

SemaphoreKind VulkanSemaphore::getKind() const {
    return m_kind;
}

std::uint64_t VulkanSemaphore::getValue() const {
    std::uint64_t value = 0;
    if (m_kind == SemaphoreKind::Timeline) {
        vkGetSemaphoreCounterValue(m_device->vkDevice(), m_semaphore, &value);
    }
    return value;
}

Result<void> VulkanSemaphore::signal(std::uint64_t value) {
    if (m_kind != SemaphoreKind::Timeline) {
        return Error{ErrorCode::InvalidArgument, "ISemaphore::signal: only Timeline semaphores can be signaled"};
    }
    // Vulkan requires a strictly increasing value; checking here turns a validation error into a Result.
    if (value <= getValue()) {
        return Error{ErrorCode::InvalidArgument, "ISemaphore::signal: value must exceed the current value"};
    }
    VkSemaphoreSignalInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
    info.semaphore = m_semaphore;
    info.value = value;
    if (VkResult result = vkSignalSemaphore(m_device->vkDevice(), &info); result != VK_SUCCESS) {
        return toError(result, "vkSignalSemaphore");
    }
    return {};
}

} // namespace rhi::vulkan
