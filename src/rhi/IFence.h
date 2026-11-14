#pragma once

#include <string>

#include "RHIFwd.h"
#include "RHITypes.h"
#include "Result.h"

namespace rhi {

struct FenceDesc {
    bool signaled = false; // create already signaled, e.g. for the first frame in flight
    std::string label;
};

// Lets the CPU wait for a queue submission to finish.
class IFence {
public:
    virtual ~IFence() = default;

    // Fails with ErrorCode::Timeout if the fence is not signaled in time.
    virtual Result<void> wait(Timeout timeout) = 0;
    virtual bool isSignaled() const = 0;
    virtual Result<void> reset() = 0;
};

} // namespace rhi
