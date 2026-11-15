#pragma once

#include <cstdint>
#include <string>

#include "RHIFwd.h"
#include "Result.h"

namespace rhi {

enum class SemaphoreKind {
    Binary,
    Timeline,
};

struct SemaphoreDesc {
    SemaphoreKind kind = SemaphoreKind::Binary;
    std::uint64_t initial_value = 0;
    std::string label;
};

class ISemaphore {
public:
    virtual ~ISemaphore() = default;

    virtual SemaphoreKind getKind() const = 0;

    virtual std::uint64_t getValue() const = 0;

    virtual Result<void> signal(std::uint64_t value) = 0;
};

} // namespace rhi
