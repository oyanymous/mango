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
    std::uint64_t value = 0;
    PipelineStage stage = PipelineStage::AllCommands;
};

struct SemaphoreSignal {
    SemaphoreRef semaphore;
    std::uint64_t value = 0;
};

struct SubmitDesc {
    std::vector<CommandBufferRef> command_buffers;
    std::vector<SemaphoreWait> waits;
    std::vector<SemaphoreSignal> signals;
    FenceRef fence;
};

struct PresentDesc {
    SwapchainRef swapchain;
    std::uint32_t image_index = 0;
    std::vector<SemaphoreRef> wait_semaphores;
};

class IQueue {
public:
    virtual ~IQueue() = default;

    virtual Result<void> submit(SubmitDesc desc) = 0;

    virtual Result<void> present(PresentDesc desc) = 0;
};

} // namespace rhi
