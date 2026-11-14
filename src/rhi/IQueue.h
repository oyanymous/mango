#pragma once

/**
 * @file
 * @brief rhi::IQueue: submits command buffers and presents swapchain images.
 */

#include <cstdint>
#include <vector>

#include "ICommandBuffer.h"
#include "IFence.h"
#include "ISemaphore.h"
#include "ISwapchain.h"
#include "RHIFwd.h"
#include "RHITypes.h"
#include "Result.h"

namespace rhi {

/**
 * @brief A semaphore a submission waits on before running.
 */
struct SemaphoreWait {
    SemaphoreRef semaphore;                           /**< The semaphore. */
    std::uint64_t value = 0;                          /**< Value to wait for (Timeline only). */
    PipelineStage stage = PipelineStage::AllCommands; /**< Stages that wait; earlier stages may start. */
};

/**
 * @brief A semaphore a submission signals when it finishes.
 */
struct SemaphoreSignal {
    SemaphoreRef semaphore;  /**< The semaphore. */
    std::uint64_t value = 0; /**< Value to set (Timeline only); must be greater than the current value. */
};

/**
 * @brief Parameters for IQueue::submit.
 */
struct SubmitDesc {
    std::vector<CommandBufferRef> command_buffers; /**< Recorded (ended) command buffers, run in order. */
    std::vector<SemaphoreWait> waits;              /**< Semaphores to wait on first. */
    std::vector<SemaphoreSignal> signals;          /**< Semaphores to signal when done. */
    FenceRef fence;                                /**< Optional; signaled when every command buffer has finished. */
};

/**
 * @brief Parameters for IQueue::present.
 */
struct PresentDesc {
    SwapchainRef swapchain;                    /**< Swapchain the image belongs to. */
    std::uint32_t image_index = 0;             /**< AcquireResult::image_index of the image. */
    std::vector<SemaphoreRef> wait_semaphores; /**< Binary semaphores signaled by the frame's submit. */
};

/**
 * @brief A GPU queue that executes command buffers. Obtained from IDevice::getQueue.
 *
 * @details
 * The queue retains submitted command buffers (and through them their resources) until they finish.
 */
class IQueue {
public:
    virtual ~IQueue() = default;

    /**
     * @brief Submits command buffers for execution.
     *
     * @param desc Command buffers, semaphores and optional fence.
     */
    virtual Result<void> submit(SubmitDesc desc) = 0;

    /**
     * @brief Queues a swapchain image for display.
     *
     * @param desc Swapchain, image and semaphores to wait on.
     * @return Success, or ErrorCode::OutOfDate when the swapchain must be resized.
     */
    virtual Result<void> present(PresentDesc desc) = 0;
};

} // namespace rhi
