#pragma once

/// @file
/// rhi::ISemaphore: GPU-side ordering between submissions and presentation.

#include <cstdint>
#include <string>

#include "RHIFwd.h"

namespace rhi {

/// The kind of semaphore.
enum class SemaphoreKind {
    Binary,   ///< Signaled once, then waited once. Required by swapchain acquire and present.
    Timeline, ///< A 64-bit counter that only increases; waits name the value to reach.
};

/// Describes a semaphore to create with IDevice::createSemaphore.
struct SemaphoreDesc {
    SemaphoreKind kind = SemaphoreKind::Binary; ///< Kind.
    std::uint64_t initial_value = 0;            ///< Starting counter value (Timeline only).
    std::string label;                          ///< Debug name.
};

/// Orders GPU work: one submission signals it, a later submission or present waits on it.
///
/// Used through SubmitDesc::waits / SubmitDesc::signals, AcquireDesc::signal_semaphore and
/// PresentDesc::wait_semaphores. The CPU cannot wait on it; use an IFence for that.
class ISemaphore {
public:
    virtual ~ISemaphore() = default;

    /// Whether the semaphore is binary or a timeline.
    virtual SemaphoreKind getKind() const = 0;
};

} // namespace rhi
