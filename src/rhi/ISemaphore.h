#pragma once

#include <cstdint>
#include <string>

#include "RHIFwd.h"

namespace rhi {

enum class SemaphoreKind {
    Binary,   // signaled once, waited once (swapchain acquire / present)
    Timeline, // monotonically increasing 64-bit value
};

struct SemaphoreDesc {
    SemaphoreKind kind = SemaphoreKind::Binary;
    std::uint64_t initial_value = 0; // Timeline only
    std::string label;
};

// GPU-to-GPU synchronization between queue submissions and presentation.
class ISemaphore {
public:
    virtual ~ISemaphore() = default;

    virtual SemaphoreKind getKind() const = 0;
};

} // namespace rhi
