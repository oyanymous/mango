#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace sg {

/**
 * An RGBA colour with channels in [0, 1] and straight (not premultiplied) alpha.
 *
 * The renderer premultiplies colours itself; everything in the scene graph API uses straight alpha.
 */
struct ColorF {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f; /**< Alpha (opacity). */


    friend bool operator==(const ColorF&, const ColorF&) = default;

    /// Opaque white, the neutral colour for multiplication.
    static constexpr ColorF white() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    /// A colour from 8-bit channels.
    static constexpr ColorF fromRgba8(int r, int g, int b, int a = 255) {
        return {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
    }
};

/// Packs a colour into 8-bit channels, R in the lowest byte (the byte order of Format::RGBA8Unorm in memory).
inline std::uint32_t packRgba8(ColorF color) {
    const auto channel = [](float v) {
        return static_cast<std::uint32_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
    };
    return channel(color.r) | (channel(color.g) << 8) | (channel(color.b) << 16) | (channel(color.a) << 24);
}

} // namespace sg
