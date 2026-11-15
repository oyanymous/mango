#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace sg {

struct ColorF {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    friend bool operator==(const ColorF&, const ColorF&) = default;

    static constexpr ColorF white() { return {1.0f, 1.0f, 1.0f, 1.0f}; }

    static constexpr ColorF fromRgba8(int r, int g, int b, int a = 255) {
        return {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
    }
};

inline std::uint32_t packRgba8(ColorF color) {
    const auto channel = [](float v) {
        return static_cast<std::uint32_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
    };
    return channel(color.r) | (channel(color.g) << 8) | (channel(color.b) << 16) | (channel(color.a) << 24);
}

} // namespace sg
