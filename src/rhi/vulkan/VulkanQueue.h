#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "IQueue.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;
class VulkanSemaphore;

/**
 * The Vulkan IQueue. Created by VulkanAdapter::createDevice, one per used queue family.
 *
 * Every submission also signals an internal timeline semaphore. Until that value is reached the queue keeps
 * the submission's command buffers, semaphores and fence alive, so callers may drop them right after submit().
 * A present's wait semaphores are kept until the same swapchain image is presented again (it must have been
 * acquired again, so the earlier present finished) or the queue is destroyed. Finished submissions are released
 * at the next submit() or present(). Not thread-safe, like the rest of the backend.
 */
class VulkanQueue final : public IQueue {
public:
    /**
     * Wraps queue 0 of a queue family and creates its timeline semaphore.
     *
     * @param type The QueueType the queue was created for.
     * @param family_index Queue family the queue belongs to.
     * @return The queue, or the error from creating the timeline semaphore.
     */
    static Result<std::shared_ptr<VulkanQueue>> create(std::shared_ptr<VulkanDeviceHandle> device, QueueType type,
                                                       std::uint32_t family_index);

    /**
     * Wraps queue 0 of a queue family. Use create().
     *
     * @param type The QueueType the queue was created for.
     * @param family_index Queue family the queue belongs to.
     * @param timeline The queue's own timeline semaphore, at value 0.
     */
    VulkanQueue(std::shared_ptr<VulkanDeviceHandle> device, QueueType type, std::uint32_t family_index,
                std::shared_ptr<VulkanSemaphore> timeline);
    /// Waits for the queue to go idle, then releases everything it still holds.
    ~VulkanQueue() override;

    VulkanQueue(const VulkanQueue&) = delete;
    VulkanQueue& operator=(const VulkanQueue&) = delete;

    /**
     * Submits with vkQueueSubmit and keeps the submission's objects alive until it finishes.
     *
     * @return InvalidArgument when a command buffer is null or not ended (a submitted one must be begun and
     *         ended again first), or a semaphore is null; otherwise the vkQueueSubmit result.
     */
    Result<void> submit(SubmitDesc desc) override;
    /**
     * Presents with vkQueuePresentKHR and keeps the wait semaphores alive (see the class details).
     *
     * @return Success, or ErrorCode::OutOfDate when the swapchain is out of date, suboptimal (the image was still
     *         presented) or has a zero extent.
     */
    Result<void> present(PresentDesc desc) override;

    /// The QueueType the queue was created for.
    QueueType type() const { return m_queue_type; }
    /// Queue family index, needed for command pools and ownership transfers.
    std::uint32_t familyIndex() const { return m_family_index; }
private:
    /// One submission's objects, held until the timeline reaches `value`.
    struct InFlight {
        std::uint64_t value = 0;                       /**< Timeline value the submission signals. */
        std::vector<CommandBufferRef> command_buffers; /**< Submitted command buffers. */
        std::vector<SemaphoreRef> semaphores;          /**< Waited and signaled semaphores. */
        FenceRef fence;                                /**< The submission's fence, if any. */
    };

    // Releases every submission the GPU finished.
    void retireFinished();

    QueueType m_queue_type;
    std::uint32_t m_family_index;
    VkQueue m_queue = VK_NULL_HANDLE;
    std::shared_ptr<VulkanSemaphore> m_timeline;
    std::uint64_t m_last_value = 0;   // the value the latest submission signals
    std::deque<InFlight> m_in_flight; // oldest first
    // Keyed by swapchain (identity only, never dereferenced) and image index.
    std::map<std::pair<const void*, std::uint32_t>, std::vector<SemaphoreRef>> m_presented;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
