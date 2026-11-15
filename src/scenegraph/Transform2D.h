#pragma once

#include <algorithm>
#include <cmath>
#include <optional>

#include "PointF.h"
#include "RectF.h"

namespace sg {

struct Transform2D {
    float a = 1.0f;
    float b = 0.0f;
    float c = 0.0f;
    float d = 1.0f;
    float tx = 0.0f;
    float ty = 0.0f;

    friend bool operator==(const Transform2D&, const Transform2D&) = default;

    static constexpr Transform2D identity() { return {}; }
    static constexpr Transform2D translation(float x, float y) { return {1.0f, 0.0f, 0.0f, 1.0f, x, y}; }

    static constexpr Transform2D scaling(float sx, float sy) { return {sx, 0.0f, 0.0f, sy, 0.0f, 0.0f}; }

    static Transform2D rotation(float radians) {
        const float cs = std::cos(radians);
        const float sn = std::sin(radians);
        return {cs, sn, -sn, cs, 0.0f, 0.0f};
    }

    friend constexpr Transform2D operator*(const Transform2D& lhs, const Transform2D& rhs) {
        return {lhs.a * rhs.a + lhs.c * rhs.b,
                lhs.b * rhs.a + lhs.d * rhs.b,
                lhs.a * rhs.c + lhs.c * rhs.d,
                lhs.b * rhs.c + lhs.d * rhs.d,
                lhs.a * rhs.tx + lhs.c * rhs.ty + lhs.tx,
                lhs.b * rhs.tx + lhs.d * rhs.ty + lhs.ty};
    }

    [[nodiscard]] constexpr PointF map(PointF p) const { return {a * p.x + c * p.y + tx, b * p.x + d * p.y + ty}; }

    [[nodiscard]] RectF mapRect(const RectF& rect) const {
        if (rect.isEmpty()) {
            return {};
        }
        const PointF p0 = map({rect.x, rect.y});
        const PointF p1 = map({rect.right(), rect.y});
        const PointF p2 = map({rect.x, rect.bottom()});
        const PointF p3 = map({rect.right(), rect.bottom()});
        const float x0 = std::min({p0.x, p1.x, p2.x, p3.x});
        const float y0 = std::min({p0.y, p1.y, p2.y, p3.y});
        const float x1 = std::max({p0.x, p1.x, p2.x, p3.x});
        const float y1 = std::max({p0.y, p1.y, p2.y, p3.y});
        return {x0, y0, x1 - x0, y1 - y0};
    }

    [[nodiscard]] constexpr bool isAxisAligned() const { return b == 0.0f && c == 0.0f; }

    [[nodiscard]] std::optional<Transform2D> inverted() const {
        const float det = a * d - b * c;
        if (std::abs(det) < 1.0e-12f) {
            return std::nullopt;
        }
        const float inv = 1.0f / det;
        return Transform2D{d * inv, -b * inv, -c * inv, a * inv, (c * ty - d * tx) * inv, (b * tx - a * ty) * inv};
    }
};

} // namespace sg
