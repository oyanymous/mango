#pragma once

/// @file
/// rhi::IFence: CPU-side waiting for GPU work.

#include <string>

#include "RHIFwd.h"
#include "RHITypes.h"
#include "Result.h"

namespace rhi {

/// Describes a fence to create with IDevice::createFence.
struct FenceDesc {
    /// Create the fence already signaled, so the first wait (e.g. for frame N - 2 in flight) returns at once.
    bool signaled = false;
    std::string label; ///< Debug name.
};

/// Lets the CPU wait until a queue submission has finished.
///
/// Pass it in SubmitDesc::fence; it becomes signaled when every command buffer of that submission completes.
/// Reset it before reusing it for another submission.
class IFence {
public:
    virtual ~IFence() = default;

    /// Blocks until the fence is signaled or @p timeout passes.
    /// @param timeout Maximum wait; kWaitForever to wait indefinitely, 0 to poll.
    /// @return Success, or an error with ErrorCode::Timeout if the fence was not signaled in time.
    virtual Result<void> wait(Timeout timeout) = 0;

    /// True if the fence is signaled. Does not block.
    virtual bool isSignaled() const = 0;

    /// Returns the fence to the unsignaled state. The fence must not be in use by a pending submission.
    virtual Result<void> reset() = 0;
};

} // namespace rhi
