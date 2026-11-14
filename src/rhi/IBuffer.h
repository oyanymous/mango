#pragma once

/// @file
/// rhi::IBuffer: linear GPU memory (vertices, indices, uniforms, storage, staging).

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "RHIFwd.h"
#include "RHITypes.h"
#include "Result.h"

namespace rhi {

/// How a buffer may be used, as a bit set. Using a buffer in a way not listed here is invalid.
enum class BufferUsage : std::uint32_t {
    None = 0,           ///< No usage.
    CopySrc = 1u << 0,  ///< Source of copies.
    CopyDst = 1u << 1,  ///< Destination of copies.
    Vertex = 1u << 2,   ///< Vertex buffer (IRenderEncoder::setVertexBuffers).
    Index = 1u << 3,    ///< Index buffer (IRenderEncoder::setIndexBuffer).
    Uniform = 1u << 4,  ///< Uniform buffer in a bind group.
    Storage = 1u << 5,  ///< Storage (read-write) buffer in a bind group.
    Indirect = 1u << 6, ///< Indirect draw/dispatch arguments.
};
MANGO_RHI_FLAGS(BufferUsage)

/// Where a buffer's memory lives, which decides whether the CPU can map it.
enum class MemoryLocation {
    GpuOnly,  ///< Fastest for the GPU; not mappable. Fill it with a copy from a CpuToGpu buffer.
    CpuToGpu, ///< Mappable, for uploads: staging buffers and per-frame uniforms.
    GpuToCpu, ///< Mappable, for reading GPU results back.
};

/// Describes a buffer to create with IDevice::createBuffer.
struct BufferDesc {
    std::uint64_t size = 0;                          ///< Size in bytes; must be > 0.
    BufferUsage usage = BufferUsage::None;           ///< Every way the buffer will be used.
    MemoryLocation memory = MemoryLocation::GpuOnly; ///< Where the memory lives.
    std::string label;                               ///< Debug name shown in tools and validation messages.
};

/// The byte range of a buffer to map.
struct MapDesc {
    std::uint64_t offset = 0;        ///< First byte.
    std::uint64_t size = kWholeSize; ///< Number of bytes; kWholeSize for the rest of the buffer.
};

/// The mapped bytes, valid until IBuffer::unmap(). Fails for GpuOnly buffers.
using MapResult = Result<std::span<std::byte>>;

/// Linear GPU memory: vertices, indices, uniforms, storage or staging data.
///
/// Created by IDevice::createBuffer. Retained by bind groups and command buffers that use it.
class IBuffer {
public:
    virtual ~IBuffer() = default;

    /// The descriptor the buffer was created with.
    virtual BufferDesc getDesc() const = 0;

    /// Maps a range of a CpuToGpu or GpuToCpu buffer into CPU memory.
    ///
    /// Only one range can be mapped at a time. The CPU must not touch memory the GPU is still using:
    /// wait on the fence of the submission that uses it first.
    /// @param desc Range to map.
    /// @return The mapped bytes, or an error (e.g. InvalidArgument for a GpuOnly buffer).
    virtual MapResult map(MapDesc desc) = 0;

    /// Ends the current mapping. CPU writes become visible to later GPU work.
    virtual void unmap() = 0;
};

} // namespace rhi
