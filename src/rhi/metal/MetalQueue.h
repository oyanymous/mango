#pragma once

/**
 * @file
 * @brief rhi::metal::MetalQueue: submissions on an `MTLCommandQueue`.
 */

#include <memory>

#include "IQueue.h"
#include "MetalCommon.h"

namespace rhi::metal {

/**
 * @brief One `MTLCommandQueue`. MetalDevice creates one per QueueType, since Metal has no queue families.
 *
 * @details
 * submit() puts semaphore waits in a small command buffer committed before the submission's own command buffers,
 * and semaphore signals plus the fence in one committed after them; Metal runs the command buffers of a queue in
 * commit order. A failed command buffer is reported as DebugSeverity::Error through the instance's debug callback,
 * and its fence is still signaled so waiters do not hang.
 */
class MetalQueue final : public IQueue {
public:
    /**
     * @brief Wraps a command queue.
     *
     * @param context Device context; kept alive by the queue.
     * @param type The queue type it serves.
     * @param queue The command queue.
     */
    MetalQueue(std::shared_ptr<MetalDeviceContext> context, QueueType type, id<MTLCommandQueue> queue);

    /**
     * @brief Commits the command buffers between the semaphore waits and signals.
     *
     * @return Success, or ErrorCode::InvalidArgument for a command buffer that is not ended, was already submitted, or
     *         was created for another queue type, a binary wait no earlier submission signals, or a fence that is
     *         signaled or pending.
     */
    Result<void> submit(SubmitDesc desc) override;

    /**
     * @brief Presents an acquired MetalSwapchain image from a small command buffer that first waits on the
     * semaphores.
     *
     * @return Success, or ErrorCode::OutOfDate if the image was not acquired (or the swapchain was resized since).
     */
    Result<void> present(PresentDesc desc) override;

    /**
     * @brief The queue type it serves.
     */
    QueueType type() const { return m_type; }

    /**
     * @brief The command queue.
     */
    id<MTLCommandQueue> handle() const { return m_queue; }
private:
    QueueType m_type;
    id<MTLCommandQueue> m_queue = nil;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
