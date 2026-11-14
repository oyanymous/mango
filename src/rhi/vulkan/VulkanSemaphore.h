#pragma once

/**
 * @file
 * @brief rhi::vulkan::VulkanSemaphore: a binary or timeline VkSemaphore.
 */

#include <memory>

#include "ISemaphore.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/**
 * @brief The Vulkan ISemaphore. Created by VulkanDevice::createSemaphore.
 */
class VulkanSemaphore final : public ISemaphore {
public:
    /**
     * @brief Creates the VkSemaphore.
     *
     * @param device Owner of the VkDevice; kept alive by the semaphore.
     * @param desc Kind and initial value.
     */
    static Result<SemaphoreRef> create(std::shared_ptr<VulkanDeviceHandle> device, SemaphoreDesc desc);

    /**
     * @brief Takes ownership of a created VkSemaphore. Use create().
     *
     * @param device Owner of the VkDevice.
     * @param kind Binary or timeline.
     * @param semaphore The semaphore.
     */
    VulkanSemaphore(std::shared_ptr<VulkanDeviceHandle> device, SemaphoreKind kind, VkSemaphore semaphore);
    /**
     * @brief Destroys the VkSemaphore.
     */
    ~VulkanSemaphore() override;

    VulkanSemaphore(const VulkanSemaphore&) = delete;
    VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;

    SemaphoreKind getKind() const override;
    /**
     * @brief Reads the counter with vkGetSemaphoreCounterValue (Timeline only).
     */
    std::uint64_t getValue() const override;

    /**
     * @brief The VkSemaphore, for VkQueue submissions.
     */
    VkSemaphore handle() const { return m_semaphore; }
private:
    SemaphoreKind m_kind;
    VkSemaphore m_semaphore = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
