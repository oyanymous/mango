#pragma once

#include <cstdint>
#include <string>

#include "RHIFwd.h"
#include "Result.h"

namespace rhi {

enum class SemaphoreKind {
    Binary,   /**< Signaled once, then waited once. Required by swapchain acquire and present. */
    Timeline, /**< A 64-bit counter that only increases; waits name the value to reach. */
};

/// Describes a semaphore to create with IDevice::createSemaphore.
struct SemaphoreDesc {
    SemaphoreKind kind = SemaphoreKind::Binary; /**< Kind. */
    std::uint64_t initial_value = 0;            /**< Starting counter value (Timeline only). */
    std::string label;
};

/**
 * Orders GPU work: one submission signals it, a later submission or present waits on it.
 *
 * Used through SubmitDesc::waits / SubmitDesc::signals, AcquireDesc::signal_semaphore and
 * PresentDesc::wait_semaphores. The CPU cannot wait on it; use an IFence for that.
 */
class ISemaphore {
public:
    virtual ~ISemaphore() = default;

    /// Whether the semaphore is binary or a timeline.
    virtual SemaphoreKind getKind() const = 0;

    /**
     * The counter value the GPU has signaled so far. Does not block.
     *
     * @return The current value for a Timeline semaphore; always 0 for a Binary one.
     */
    virtual std::uint64_t getValue() const = 0;
    /**
     * Sets a Timeline semaphore's counter from the CPU, releasing GPU waits for values up to @p value.
     *
     * @param value The new value; must be greater than the current one.
     * @return InvalidArgument for a Binary semaphore or a value not above the current one.
     */
    virtual Result<void> signal(std::uint64_t value) = 0;
};

} // namespace rhi
