#include "MetalFence.h"

#include <algorithm>

namespace rhi::metal {

MetalFence::MetalFence(FenceDesc desc) : m_signaled(desc.signaled) {
}

Result<void> MetalFence::wait(Timeout timeout) {
    std::unique_lock lock(m_mutex);
    const auto signaled = [this] { return m_signaled; };
    if (timeout == kWaitForever) {
        m_signaled_cv.wait(lock, signaled);
        return {};
    }

    if (!m_signaled_cv.wait_for(lock, std::max(timeout, Timeout::zero()), signaled)) {
        return Error{ErrorCode::Timeout, "MetalFence::wait timed out"};
    }
    return {};
}

bool MetalFence::isSignaled() const {
    std::lock_guard lock(m_mutex);
    return m_signaled;
}

Result<void> MetalFence::reset() {
    std::lock_guard lock(m_mutex);
    if (m_pending) {
        return Error{ErrorCode::InvalidArgument, "MetalFence::reset: the fence is in use by a pending submission"};
    }
    m_signaled = false;
    return {};
}

Result<void> MetalFence::markPending() {
    std::lock_guard lock(m_mutex);
    if (m_signaled || m_pending) {
        return Error{ErrorCode::InvalidArgument,
                     "MetalQueue::submit: the fence is signaled or already pending; reset it after its last wait"};
    }
    m_pending = true;
    return {};
}

void MetalFence::signal() {
    {
        std::lock_guard lock(m_mutex);
        m_signaled = true;
        m_pending = false;
    }
    m_signaled_cv.notify_all();
}

} // namespace rhi::metal
