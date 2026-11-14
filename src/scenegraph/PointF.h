#pragma once

/**
 * @file
 * @brief sg::PointF: a 2D point (or vector) with float coordinates.
 */

namespace sg {

/**
 * @brief A 2D point or vector in logical units (or pixels, where documented), +Y pointing down.
 */
struct PointF {
    float x = 0.0f; /**< X coordinate. */
    float y = 0.0f; /**< Y coordinate. */

    /** @brief Member-wise equality. */

    friend bool operator==(const PointF&, const PointF&) = default;
    /** @brief Component-wise sum. */
    friend PointF operator+(PointF a, PointF b) { return {a.x + b.x, a.y + b.y}; }
    /** @brief Component-wise difference. */
    friend PointF operator-(PointF a, PointF b) { return {a.x - b.x, a.y - b.y}; }
    /** @brief Scaled by @p s. */
    friend PointF operator*(PointF a, float s) { return {a.x * s, a.y * s}; }
};

} // namespace sg
