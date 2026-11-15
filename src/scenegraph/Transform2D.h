#pragma once

#include <algorithm>
#include <cmath>
#include <optional>

#include "PointF.h"
#include "RectF.h"

namespace sg {

/**
 * A 2D affine transform, `x' = a*x + c*y + tx`, `y' = b*x + d*y + ty` (the CSS `matrix(a, b, c, d, tx, ty)`).
 *
 * Composition reads right to left, as with matrices: `(A * B).map(p) == A.map(B.map(p))`. With +Y pointing down a
 * positive rotation turns clockwise on screen.
 */
struct Transform2D {
    float a = 1.0f;  /**< X scale / rotation. */
    float b = 0.0f;  /**< Y shear / rotation. */
    float c = 0.0f;  /**< X shear / rotation. */
    float d = 1.0f;  /**< Y scale / rotation. */
    float tx = 0.0f; /**< X translation. */
    float ty = 0.0f; /**< Y translation. */


    friend bool operator==(const Transform2D&, const Transform2D&) = default;

    static constexpr Transform2D identity() { return {}; }
    static constexpr Transform2D translation(float x, float y) { return {1.0f, 0.0f, 0.0f, 1.0f, x, y}; }
    /// A scale by @p sx horizontally and @p sy vertically, about the origin.
    static constexpr Transform2D scaling(float sx, float sy) { return {sx, 0.0f, 0.0f, sy, 0.0f, 0.0f}; }
    /// A rotation by @p radians about the origin (clockwise on screen, since +Y points down).
    static Transform2D rotation(float radians) {
        const float cs = std::cos(radians);
        const float sn = std::sin(radians);
        return {cs, sn, -sn, cs, 0.0f, 0.0f};
    }

    /// The composition: applies @p rhs first, then @p lhs.
    friend constexpr Transform2D operator*(const Transform2D& lhs, const Transform2D& rhs) {
        return {lhs.a * rhs.a + lhs.c * rhs.b,
                lhs.b * rhs.a + lhs.d * rhs.b,
                lhs.a * rhs.c + lhs.c * rhs.d,
                lhs.b * rhs.c + lhs.d * rhs.d,
                lhs.a * rhs.tx + lhs.c * rhs.ty + lhs.tx,
                lhs.b * rhs.tx + lhs.d * rhs.ty + lhs.ty};
    }

    [[nodiscard]] constexpr PointF map(PointF p) const { return {a * p.x + c * p.y + tx, b * p.x + d * p.y + ty}; }

    /// The bounding box of @p rect after mapping its four corners; empty for an empty rectangle.
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

    /// Whether axis-aligned rectangles stay axis-aligned (no rotation or shear; flips and scales allowed).
    [[nodiscard]] constexpr bool isAxisAligned() const { return b == 0.0f && c == 0.0f; }

    /// The inverse transform, or nothing when the transform is singular (e.g. a zero scale).
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
