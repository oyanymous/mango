/**
 * @file
 * @brief MetalQueue: submission with `MTLSharedEvent` waits/signals and completion-handler fences.
 */

#include "MetalQueue.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "MetalFence.h"
#include "MetalSemaphore.h"

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

Result<void> MetalQueue::submit(SubmitDesc desc) {
    if (!desc.command_buffers.empty()) {
        return Error{ErrorCode::Unsupported, "MetalQueue::submit: command buffers are not implemented yet"};
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

    if (waits.empty() && signals.empty() && !fence) {
        return {};
    }

    // Waits and signals go into internal command buffers; with no command buffers of the caller's, one is enough.
    id<MTLCommandBuffer> sync = [m_queue commandBuffer];
    sync.label = @"MetalQueue::submit sync";
    for (const EventValue& wait : waits) {
        [sync encodeWaitForEvent:wait.event value:wait.value];
    }
    for (const EventValue& signal : signals) {
        [sync encodeSignalEvent:signal.event value:signal.value];
    }
    std::shared_ptr<MetalDebugReporter> reporter = m_context->reporter;
    [sync addCompletedHandler:^(id<MTLCommandBuffer> completed) {
        reportFailure(*reporter, completed);
        if (fence) {
            fence->signal();
        }
    }];
    [sync commit];
    return {};
}

Result<void> MetalQueue::present(PresentDesc) {
    return Error{ErrorCode::Unsupported, "MetalQueue::present is not implemented yet"};
}

} // namespace rhi::metal
