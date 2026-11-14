#pragma once

#include <cstdint>

namespace rhi {

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

} // namespace rhi
