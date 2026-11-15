#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "PointF.h"

namespace sg {

struct RectF {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    friend bool operator==(const RectF&, const RectF&) = default;

    static constexpr RectF infinite() { return {-1.0e7f, -1.0e7f, 2.0e7f, 2.0e7f}; }

    static RectF fromCorners(PointF a, PointF b) {
        const float x0 = std::min(a.x, b.x);
        const float y0 = std::min(a.y, b.y);
        return {x0, y0, std::max(a.x, b.x) - x0, std::max(a.y, b.y) - y0};
    }

    [[nodiscard]] bool isEmpty() const { return !(width > 0.0f) || !(height > 0.0f); }
    [[nodiscard]] float right() const { return x + width; }
    [[nodiscard]] float bottom() const { return y + height; }

    [[nodiscard]] RectF intersected(const RectF& other) const {
        const float x0 = std::max(x, other.x);
        const float y0 = std::max(y, other.y);
        const float x1 = std::min(right(), other.right());
        const float y1 = std::min(bottom(), other.bottom());
        if (x1 <= x0 || y1 <= y0) {
            return {};
        }
        return {x0, y0, x1 - x0, y1 - y0};
    }

    [[nodiscard]] RectF united(const RectF& other) const {
        if (isEmpty()) {
            return other;
        }
        if (other.isEmpty()) {
            return *this;
        }
        const float x0 = std::min(x, other.x);
        const float y0 = std::min(y, other.y);
        return {x0, y0, std::max(right(), other.right()) - x0, std::max(bottom(), other.bottom()) - y0};
    }

    [[nodiscard]] RectF adjusted(float amount) const {
        return {x - amount, y - amount, width + 2.0f * amount, height + 2.0f * amount};
    }

    [[nodiscard]] bool contains(PointF p) const { return p.x >= x && p.y >= y && p.x < right() && p.y < bottom(); }
};

struct IntRect {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;

    friend bool operator==(const IntRect&, const IntRect&) = default;

    static IntRect covering(const RectF& rect) {
        if (rect.isEmpty()) {
            return {};
        }
        constexpr float kLimit = 1.0e7f;
        const float x0 = std::floor(std::clamp(rect.x, -kLimit, kLimit));
        const float y0 = std::floor(std::clamp(rect.y, -kLimit, kLimit));
        const float x1 = std::ceil(std::clamp(rect.right(), -kLimit, kLimit));
        const float y1 = std::ceil(std::clamp(rect.bottom(), -kLimit, kLimit));
        return {static_cast<std::int32_t>(x0), static_cast<std::int32_t>(y0), static_cast<std::int32_t>(x1 - x0),
                static_cast<std::int32_t>(y1 - y0)};
    }

    [[nodiscard]] bool isEmpty() const { return width <= 0 || height <= 0; }
    [[nodiscard]] std::int32_t right() const { return x + width; }
    [[nodiscard]] std::int32_t bottom() const { return y + height; }

    [[nodiscard]] std::int64_t area() const {
        return isEmpty() ? 0 : static_cast<std::int64_t>(width) * static_cast<std::int64_t>(height);
    }

    [[nodiscard]] IntRect intersected(const IntRect& other) const {
        const std::int32_t x0 = std::max(x, other.x);
        const std::int32_t y0 = std::max(y, other.y);
        const std::int32_t x1 = std::min(right(), other.right());
        const std::int32_t y1 = std::min(bottom(), other.bottom());
        if (x1 <= x0 || y1 <= y0) {
            return {};
        }
        return {x0, y0, x1 - x0, y1 - y0};
    }

    [[nodiscard]] IntRect united(const IntRect& other) const {
        if (isEmpty()) {
            return other;
        }
        if (other.isEmpty()) {
            return *this;
        }
        const std::int32_t x0 = std::min(x, other.x);
        const std::int32_t y0 = std::min(y, other.y);
        return {x0, y0, std::max(right(), other.right()) - x0, std::max(bottom(), other.bottom()) - y0};
    }

    [[nodiscard]] bool intersects(const IntRect& other) const { return !intersected(other).isEmpty(); }

    [[nodiscard]] bool contains(const IntRect& other) const {
        return other.x >= x && other.y >= y && other.right() <= right() && other.bottom() <= bottom();
    }
};

} // namespace sg
