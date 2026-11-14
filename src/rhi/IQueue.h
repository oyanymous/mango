#pragma once

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

struct SemaphoreWait {
    SemaphoreRef semaphore;
    std::uint64_t value = 0;                          // Timeline only
    PipelineStage stage = PipelineStage::AllCommands; // the stages that wait
};

struct SemaphoreSignal {
    SemaphoreRef semaphore;
    std::uint64_t value = 0; // Timeline only
};

struct SubmitDesc {
    std::vector<CommandBufferRef> command_buffers;
    std::vector<SemaphoreWait> waits;
    std::vector<SemaphoreSignal> signals;
    FenceRef fence; // optional; signaled when every command buffer has finished
};

struct PresentDesc {
    SwapchainRef swapchain;
    std::uint32_t image_index = 0;
    std::vector<SemaphoreRef> wait_semaphores; // binary
};

// Executes command buffers. The queue retains submitted command buffers until they finish.
class IQueue {
public:
    virtual ~IQueue() = default;

    virtual Result<void> submit(SubmitDesc desc) = 0;
    // Fails with ErrorCode::OutOfDate when the swapchain must be resized.
    virtual Result<void> present(PresentDesc desc) = 0;
};

} // namespace rhi
