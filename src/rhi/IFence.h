#pragma once

#include <string>

#include "RHIFwd.h"
#include "RHITypes.h"
#include "Result.h"

namespace rhi {

struct FenceDesc {
    bool signaled = false;
    std::string label;
};

class IFence {
public:
    virtual ~IFence() = default;

    virtual Result<void> wait(Timeout timeout) = 0;

    virtual bool isSignaled() const = 0;

    virtual Result<void> reset() = 0;
};

} // namespace rhi
