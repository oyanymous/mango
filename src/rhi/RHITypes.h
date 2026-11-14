#pragma once

/// @file
/// Small value types and enums shared across the RHI: extents, formats, stages, flags.

#include <chrono>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace rhi {

/// Defines `|`, `&`, `|=` and `hasFlag()` for an `enum class` used as a bit set.
///
/// Use it right after the enum: `MANGO_RHI_FLAGS(BufferUsage)`.
/// @code
/// BufferUsage usage = BufferUsage::Vertex | BufferUsage::CopyDst;
/// if (hasFlag(usage, BufferUsage::Vertex)) { ... }
/// @endcode
#define MANGO_RHI_FLAGS(Type)                                                                                          \
    constexpr Type operator|(Type a, Type b) {                                                                         \
        return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(a) |                                        \
                                 static_cast<std::underlying_type_t<Type>>(b));                                        \
    }                                                                                                                  \
    constexpr Type operator&(Type a, Type b) {                                                                         \
        return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(a) &                                        \
                                 static_cast<std::underlying_type_t<Type>>(b));                                        \
    }                                                                                                                  \
    constexpr Type& operator|=(Type& a, Type b) {                                                                      \
        return a = a | b;                                                                                              \
    }                                                                                                                  \
    constexpr bool hasFlag(Type value, Type flag) {                                                                    \
        return (value & flag) == flag;                                                                                 \
    }

/// Pass as a size to mean "from the offset to the end of the resource".
inline constexpr std::uint64_t kWholeSize = std::numeric_limits<std::uint64_t>::max();
/// Pass as a mip or layer count to mean "all remaining mip levels / array layers".
inline constexpr std::uint32_t kRemaining = std::numeric_limits<std::uint32_t>::max();

/// How long a CPU wait may block.
using Timeout = std::chrono::nanoseconds;
/// A Timeout that never expires.
inline constexpr Timeout kWaitForever = Timeout::max();

/// A 2D size in texels or pixels.
struct Extent2D {
    std::uint32_t width = 0;  ///< Width.
    std::uint32_t height = 0; ///< Height.

    /// Member-wise equality.
    friend bool operator==(const Extent2D&, const Extent2D&) = default;
};

/// A 3D size in texels. `depth` is 1 for 2D textures.
struct Extent3D {
    std::uint32_t width = 0;  ///< Width.
    std::uint32_t height = 0; ///< Height.
    std::uint32_t depth = 1;  ///< Depth (1 for 2D).

    /// Member-wise equality.
    friend bool operator==(const Extent3D&, const Extent3D&) = default;
};

/// A texel offset within a texture.
struct Offset3D {
    std::int32_t x = 0; ///< X offset.
    std::int32_t y = 0; ///< Y offset.
    std::int32_t z = 0; ///< Z offset (0 for 2D).

    /// Member-wise equality.
    friend bool operator==(const Offset3D&, const Offset3D&) = default;
};

/// The kinds of GPU queues. A device may back several kinds with the same hardware queue.
enum class QueueType {
    Graphics, ///< Graphics, compute, transfer and present.
    Compute,  ///< Compute and transfer; may run alongside graphics.
    Transfer, ///< Copies only; may run alongside the other queues.
};

/// Texel formats. The names list channels in memory order, then the type (Unorm = normalized unsigned,
/// Srgb = sRGB-encoded, Float, Uint); D/S formats are depth/stencil.
enum class Format {
    Undefined, ///< No format; in a view or attachment it means "same as the texture".

    R8Unorm,    ///< One 8-bit normalized channel.
    RG8Unorm,   ///< Two 8-bit normalized channels.
    RGBA8Unorm, ///< Four 8-bit normalized channels.
    RGBA8Srgb,  ///< Four 8-bit channels, RGB sRGB-encoded.
    BGRA8Unorm, ///< Four 8-bit normalized channels in BGRA order (common swapchain format).
    BGRA8Srgb,  ///< BGRA, sRGB-encoded (common swapchain format).

    R16Float,    ///< One 16-bit float channel.
    RG16Float,   ///< Two 16-bit float channels.
    RGBA16Float, ///< Four 16-bit float channels (HDR colour).

    R32Uint,     ///< One 32-bit unsigned integer channel.
    R32Float,    ///< One 32-bit float channel.
    RG32Float,   ///< Two 32-bit float channels.
    RGB32Float,  ///< Three 32-bit float channels (vertex data; rarely a texture format).
    RGBA32Float, ///< Four 32-bit float channels.

    D16Unorm,       ///< 16-bit normalized depth.
    D24UnormS8Uint, ///< 24-bit depth + 8-bit stencil. Not supported on every GPU (e.g. Apple).
    D32Float,       ///< 32-bit float depth.
    D32FloatS8Uint, ///< 32-bit float depth + 8-bit stencil.
};

/// The type of each index in an index buffer.
enum class IndexFormat {
    Uint16, ///< 16-bit indices.
    Uint32, ///< 32-bit indices.
};

/// A comparison between a new value and a stored one, for depth tests and comparison samplers.
enum class CompareOp {
    Never,          ///< Never passes.
    Less,           ///< Passes if new < stored.
    Equal,          ///< Passes if new == stored.
    LessOrEqual,    ///< Passes if new <= stored.
    Greater,        ///< Passes if new > stored.
    NotEqual,       ///< Passes if new != stored.
    GreaterOrEqual, ///< Passes if new >= stored.
    Always,         ///< Always passes.
};

/// Shader stages, as a bit set (see MANGO_RHI_FLAGS).
enum class ShaderStage : std::uint32_t {
    None = 0,                        ///< No stage.
    Vertex = 1u << 0,                ///< Vertex shader.
    Fragment = 1u << 1,              ///< Fragment (pixel) shader.
    Compute = 1u << 2,               ///< Compute shader.
    AllGraphics = Vertex | Fragment, ///< Every graphics stage.
};
MANGO_RHI_FLAGS(ShaderStage)

/// GPU pipeline stages, as a bit set. Used where a queue submission waits on a semaphore (SemaphoreWait).
enum class PipelineStage : std::uint32_t {
    None = 0,                     ///< No stage.
    VertexInput = 1u << 0,        ///< Vertex and index buffer reads.
    VertexShader = 1u << 1,       ///< Vertex shading.
    FragmentShader = 1u << 2,     ///< Fragment shading.
    ComputeShader = 1u << 3,      ///< Compute shading.
    RenderTargetOutput = 1u << 4, ///< Colour attachment writes; wait here for a swapchain image.
    DepthStencil = 1u << 5,       ///< Depth/stencil tests and writes.
    Copy = 1u << 6,               ///< Copy commands.
    AllCommands = 1u << 7,        ///< Everything.
};
MANGO_RHI_FLAGS(PipelineStage)

/// The mip levels and array layers of a texture that a view or barrier covers.
struct TextureSubresourceRange {
    std::uint32_t base_mip_level = 0;             ///< First mip level.
    std::uint32_t mip_level_count = kRemaining;   ///< Number of mip levels; kRemaining for all from the base.
    std::uint32_t base_array_layer = 0;           ///< First array layer.
    std::uint32_t array_layer_count = kRemaining; ///< Number of layers; kRemaining for all from the base.
};

} // namespace rhi
