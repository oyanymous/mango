#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "RHIFwd.h"
#include "RHITypes.h"
#include "Result.h"

namespace rhi {

enum class BufferUsage : std::uint32_t {
    None = 0,
    CopySrc = 1u << 0,
    CopyDst = 1u << 1,
    Vertex = 1u << 2,
    Index = 1u << 3,
    Uniform = 1u << 4,
    Storage = 1u << 5,
    Indirect = 1u << 6,
};
MANGO_RHI_FLAGS(BufferUsage)

// Where the buffer's memory lives, which decides whether the CPU can map it.
enum class MemoryLocation {
    GpuOnly,  // fastest for the GPU; not mappable
    CpuToGpu, // mappable, for uploads (staging, per-frame uniforms)
    GpuToCpu, // mappable, for readbacks
};

struct BufferDesc {
    std::uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
    MemoryLocation memory = MemoryLocation::GpuOnly;
    std::string label;
};

struct MapDesc {
    std::uint64_t offset = 0;
    std::uint64_t size = kWholeSize;
};

// The mapped bytes, valid until unmap(). Fails for GpuOnly buffers.
using MapResult = Result<std::span<std::byte>>;

class IBuffer {
public:
    virtual ~IBuffer() = default;

    virtual BufferDesc getDesc() const = 0;
    virtual MapResult map(MapDesc desc) = 0;
    virtual void unmap() = 0;
};

} // namespace rhi
