/**
 * @file
 * @brief MetalQueue: submission with `MTLSharedEvent` waits/signals and completion-handler fences.
 */

#include "MetalQueue.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "MetalCommandBuffer.h"
#include "MetalFence.h"
#include "MetalSemaphore.h"
#include "MetalSwapchain.h"

namespace rhi::metal {

namespace {

/**
 * @brief An event and the value to wait for or signal.
 */
struct EventValue {
    id<MTLSharedEvent> event = nil;
    std::uint64_t value = 0;
};

/**
 * @brief Reports @p command_buffer's error, if it failed, as DebugSeverity::Error. Runs in a completion handler.
 */
void reportFailure(const MetalDebugReporter& reporter, id<MTLCommandBuffer> command_buffer) {
    if (command_buffer.status != MTLCommandBufferStatusError) {
        return;
    }
    NSString* label = command_buffer.label ?: @"unlabeled";
    NSString* reason = command_buffer.error.localizedDescription ?: @"unknown error";
    reporter.report(DebugSeverity::Error,
                    std::string("Metal command buffer '") + label.UTF8String + "' failed: " + reason.UTF8String);
}

} // namespace

MetalQueue::MetalQueue(std::shared_ptr<MetalDeviceContext> context, QueueType type, id<MTLCommandQueue> queue)
    : m_type(type), m_queue(queue), m_context(std::move(context)) {
}

MetalQueue::~MetalQueue() {
    // A queue runs its command buffers in commit order, so an empty one committed last finishes after the others.
    id<MTLCommandBuffer> drain = [m_queue commandBuffer];
    drain.label = @"MetalQueue drain";
    [drain commit];
    [drain waitUntilCompleted];
}

Result<void> MetalQueue::submit(SubmitDesc desc) {
    std::vector<std::shared_ptr<MetalCommandBuffer>> command_buffers;
    for (const CommandBufferRef& ref : desc.command_buffers) {
        auto command_buffer = std::static_pointer_cast<MetalCommandBuffer>(ref);
        if (!command_buffer || command_buffer->state() != MetalCommandBuffer::RecordingState::Executable) {
            return Error{ErrorCode::InvalidArgument,
                         "MetalQueue::submit: command buffer is null, not ended, or already submitted"};
        }
        if (command_buffer->queue() != m_queue) {
            return Error{ErrorCode::InvalidArgument,
                         "MetalQueue::submit: command buffer was created for another queue type"};
        }
        command_buffers.push_back(std::move(command_buffer));
    }
    for (const SemaphoreWait& wait : desc.waits) {
        if (!wait.semaphore) {
            return Error{ErrorCode::InvalidArgument, "MetalQueue::submit: null wait semaphore"};
        }
    }
    for (const SemaphoreSignal& signal : desc.signals) {
        if (!signal.semaphore) {
            return Error{ErrorCode::InvalidArgument, "MetalQueue::submit: null signal semaphore"};
        }
    }

    // Claim binary wait numbers first: it is the only step that can fail, so nothing else changes state on error.
    std::vector<EventValue> waits;
    for (const SemaphoreWait& wait : desc.waits) {
        auto& semaphore = static_cast<MetalSemaphore&>(*wait.semaphore);
        Result<std::uint64_t> value = semaphore.nextWaitValue(wait.value);
        if (!value) {
            return value.error();
        }
        waits.push_back({semaphore.event(), *value});
    }
    std::shared_ptr<MetalFence> fence = std::static_pointer_cast<MetalFence>(desc.fence);
    if (fence) {
        if (Result<void> pending = fence->markPending(); !pending) {
            return pending;
        }
    }
    std::vector<EventValue> signals;
    for (const SemaphoreSignal& signal : desc.signals) {
        auto& semaphore = static_cast<MetalSemaphore&>(*signal.semaphore);
        signals.push_back({semaphore.event(), semaphore.nextSignalValue(signal.value)});
    }

    // The queue runs command buffers in commit order: semaphore waits in a leading internal command buffer, then the
    // caller's, then signals in a trailing one. Without command buffers of the caller's, one internal buffer does both.
    std::vector<id<MTLCommandBuffer>> sequence;
    std::vector<std::shared_ptr<MetalCommandBuffer>> owners; // parallel to sequence; null for internal ones
    const auto addInternal = [&](NSString* label) {
        id<MTLCommandBuffer> internal = [m_queue commandBuffer];
        internal.label = label;
        sequence.push_back(internal);
        owners.push_back(nullptr);
        return internal;
    };
    id<MTLCommandBuffer> waits_buffer = waits.empty() ? nil : addInternal(@"MetalQueue::submit waits");
    for (const EventValue& wait : waits) {
        [waits_buffer encodeWaitForEvent:wait.event value:wait.value];
    }
    for (const std::shared_ptr<MetalCommandBuffer>& command_buffer : command_buffers) {
        sequence.push_back(command_buffer->handle());
        owners.push_back(command_buffer);
    }
    if (!signals.empty() || (sequence.empty() && fence)) {
        id<MTLCommandBuffer> signals_buffer =
            command_buffers.empty() && waits_buffer != nil ? waits_buffer : addInternal(@"MetalQueue::submit signals");
        for (const EventValue& signal : signals) {
            [signals_buffer encodeSignalEvent:signal.event value:signal.value];
        }
    }
    if (sequence.empty()) {
        return {};
    }

    // Every command buffer reports its own failure and keeps its RHI object alive until it completes. The fence waits
    // for all of them, so it does not depend on the order completion handlers run in.
    // The semaphores stay alive until the last command buffer completes, like everything else the submission uses.
    auto remaining = std::make_shared<std::atomic<std::size_t>>(sequence.size());
    auto semaphores = std::make_shared<std::vector<SemaphoreRef>>();
    for (const SemaphoreWait& wait : desc.waits) {
        semaphores->push_back(wait.semaphore);
    }
    for (const SemaphoreSignal& signal : desc.signals) {
        semaphores->push_back(signal.semaphore);
    }
    std::shared_ptr<MetalDebugReporter> reporter = m_context->reporter;
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        std::shared_ptr<MetalCommandBuffer> owner = owners[i];
        [sequence[i] addCompletedHandler:^(id<MTLCommandBuffer> completed) {
            reportFailure(*reporter, completed);
            if (owner) {
                owner->markCompleted();
            }
            if (remaining->fetch_sub(1) == 1) {
                semaphores->clear();
                if (fence) {
                    fence->signal();
                }
            }
        }];
    }
    for (id<MTLCommandBuffer> command_buffer : sequence) {
        [command_buffer commit];
    }
    for (const std::shared_ptr<MetalCommandBuffer>& command_buffer : command_buffers) {
        command_buffer->markSubmitted();
    }
    return {};
}

Result<void> MetalQueue::present(PresentDesc desc) {
    if (!desc.swapchain) {
        return Error{ErrorCode::InvalidArgument, "MetalQueue::present: swapchain is null"};
    }
    for (const SemaphoreRef& semaphore : desc.wait_semaphores) {
        if (!semaphore) {
            return Error{ErrorCode::InvalidArgument, "MetalQueue::present: null wait semaphore"};
        }
    }
    std::vector<EventValue> waits;
    for (const SemaphoreRef& semaphore : desc.wait_semaphores) {
        auto& metal_semaphore = static_cast<MetalSemaphore&>(*semaphore);
        Result<std::uint64_t> value = metal_semaphore.nextWaitValue(0);
        if (!value) {
            return value.error();
        }
        waits.push_back({metal_semaphore.event(), *value});
    }
    id<CAMetalDrawable> drawable = static_cast<MetalSwapchain&>(*desc.swapchain).takeDrawable(desc.image_index);
    if (drawable == nil) {
        return Error{ErrorCode::OutOfDate, "MetalQueue::present: the image was not acquired, or the swapchain resized"};
    }

    // Presentation is scheduled from a command buffer, after the frame's work it waits on.
    id<MTLCommandBuffer> command_buffer = [m_queue commandBuffer];
    command_buffer.label = @"MetalQueue::present";
    for (const EventValue& wait : waits) {
        [command_buffer encodeWaitForEvent:wait.event value:wait.value];
    }
    [command_buffer presentDrawable:drawable];
    std::shared_ptr<MetalDebugReporter> reporter = m_context->reporter;
    auto semaphores = std::make_shared<std::vector<SemaphoreRef>>(std::move(desc.wait_semaphores));
    [command_buffer addCompletedHandler:^(id<MTLCommandBuffer> completed) {
        reportFailure(*reporter, completed);
        semaphores->clear();
    }];
    [command_buffer commit];
    return {};
}

} // namespace rhi::metal
