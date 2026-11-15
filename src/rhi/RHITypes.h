#pragma once

#include <chrono>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace rhi {

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

inline constexpr std::uint64_t kWholeSize = std::numeric_limits<std::uint64_t>::max();

inline constexpr std::uint32_t kRemaining = std::numeric_limits<std::uint32_t>::max();

using Timeout = std::chrono::nanoseconds;

inline constexpr Timeout kWaitForever = Timeout::max();

struct Extent2D {
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    friend bool operator==(const Extent2D&, const Extent2D&) = default;
};

struct Extent3D {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t depth = 1;

    friend bool operator==(const Extent3D&, const Extent3D&) = default;
};

struct Offset3D {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t z = 0;

    friend bool operator==(const Offset3D&, const Offset3D&) = default;
};

enum class QueueType {
    Graphics,
    Compute,
    Transfer,
};

enum class Format {
    Undefined,

    R8Unorm,
    RG8Unorm,
    RGBA8Unorm,
    RGBA8Srgb,
    BGRA8Unorm,
    BGRA8Srgb,

    R16Float,
    RG16Float,
    RGBA16Float,

    R32Uint,
    R32Float,
    RG32Float,
    RGB32Float,
    RGBA32Float,

    D16Unorm,
    D24UnormS8Uint,
    D32Float,
    D32FloatS8Uint,
};

enum class IndexFormat {
    Uint16,
    Uint32,
};

enum class CompareOp {
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always,
};

enum class ShaderStage : std::uint32_t {
    None = 0,
    Vertex = 1u << 0,
    Fragment = 1u << 1,
    Compute = 1u << 2,
    AllGraphics = Vertex | Fragment,
};
MANGO_RHI_FLAGS(ShaderStage)

enum class PipelineStage : std::uint32_t {
    None = 0,
    VertexInput = 1u << 0,
    VertexShader = 1u << 1,
    FragmentShader = 1u << 2,
    ComputeShader = 1u << 3,
    RenderTargetOutput = 1u << 4,
    DepthStencil = 1u << 5,
    Copy = 1u << 6,
    AllCommands = 1u << 7,
};
MANGO_RHI_FLAGS(PipelineStage)

struct TextureSubresourceRange {
    std::uint32_t base_mip_level = 0;
    std::uint32_t mip_level_count = kRemaining;
    std::uint32_t base_array_layer = 0;
    std::uint32_t array_layer_count = kRemaining;
};

} // namespace rhi
