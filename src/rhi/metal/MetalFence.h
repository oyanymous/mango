#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>

#include "IFence.h"
#include "MetalCommon.h"

namespace rhi::metal {

/**
 * A fence signaled from a command buffer's completion handler.
 *
 * Metal has no fence object the CPU can wait on (`MTLSharedEvent` waits need macOS 14), so the state lives in a
 * mutex-guarded flag. MetalQueue::submit adds a completion handler that calls signal(); the handler runs on a Metal
 * thread, so every member is thread-safe.
 */
class MetalFence final : public IFence {
public:
    explicit MetalFence(FenceDesc desc);

    Result<void> wait(Timeout timeout) override;
    bool isSignaled() const override;
    Result<void> reset() override;

    /// Marks the fence as owned by a pending submission. Fails if it is signaled or already pending.
    Result<void> markPending();

    /// Marks the fence signaled and wakes waiters. Called from the completion handler, on any thread.
    void signal();
private:
    mutable std::mutex m_mutex;
    std::condition_variable m_signaled_cv;
    bool m_signaled = false;
    bool m_pending = false; // a submission will signal it
};

} // namespace rhi::metal
