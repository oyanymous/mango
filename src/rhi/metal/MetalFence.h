#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>

#include "IFence.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalFence final : public IFence {
public:
    explicit MetalFence(FenceDesc desc);

    Result<void> wait(Timeout timeout) override;
    bool isSignaled() const override;
    Result<void> reset() override;

    Result<void> markPending();

    void signal();
private:
    mutable std::mutex m_mutex;
    std::condition_variable m_signaled_cv;
    bool m_signaled = false;
    bool m_pending = false;
};

} // namespace rhi::metal
