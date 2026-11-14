#pragma once

/**
 * @file
 * @brief sg::RectF and sg::IntRect: axis-aligned rectangles with float and integer coordinates.
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "PointF.h"

namespace sg {

/**
 * @brief An axis-aligned rectangle with float coordinates. Empty when its width or height is not positive.
 */
struct RectF {
    float x = 0.0f;      /**< Left edge. */
    float y = 0.0f;      /**< Top edge. */
    float width = 0.0f;  /**< Width; the rectangle is empty when <= 0. */
    float height = 0.0f; /**< Height; the rectangle is empty when <= 0. */

    /** @brief Member-wise equality. */

    friend bool operator==(const RectF&, const RectF&) = default;

    /**
     * @brief A rectangle larger than any render target, used for "no clip".
     */
    static constexpr RectF infinite() { return {-1.0e7f, -1.0e7f, 2.0e7f, 2.0e7f}; }

    /**
     * @brief The rectangle spanning two corners, in any order.
     */
    static RectF fromCorners(PointF a, PointF b) {
        const float x0 = std::min(a.x, b.x);
        const float y0 = std::min(a.y, b.y);
        return {x0, y0, std::max(a.x, b.x) - x0, std::max(a.y, b.y) - y0};
    }

    [[nodiscard]] bool isEmpty() const { return !(width > 0.0f) || !(height > 0.0f); } /**< No area (or NaN). */
    [[nodiscard]] float right() const { return x + width; }                            /**< x + width. */
    [[nodiscard]] float bottom() const { return y + height; }                          /**< y + height. */

    /**
     * @brief The overlap of two rectangles; empty (all zero) when they do not overlap.
     */
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

    /**
     * @brief The smallest rectangle containing both; an empty rectangle is ignored.
     */
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

    /**
     * @brief The rectangle grown by @p amount on every side.
     */
    [[nodiscard]] RectF adjusted(float amount) const {
        return {x - amount, y - amount, width + 2.0f * amount, height + 2.0f * amount};
    }

    /**
     * @brief Whether @p p lies inside (left/top edges inclusive, right/bottom exclusive).
     */
    [[nodiscard]] bool contains(PointF p) const { return p.x >= x && p.y >= y && p.x < right() && p.y < bottom(); }
};

/**
 * @brief An axis-aligned rectangle of whole pixels. Empty when its width or height is not positive.
 */
struct IntRect {
    std::int32_t x = 0;      /**< Left edge. */
    std::int32_t y = 0;      /**< Top edge. */
    std::int32_t width = 0;  /**< Width. */
    std::int32_t height = 0; /**< Height. */

    /** @brief Member-wise equality. */

    friend bool operator==(const IntRect&, const IntRect&) = default;

    /**
     * @brief The smallest pixel rectangle covering @p rect (edges rounded outwards); empty for an empty rect.
     */
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

    [[nodiscard]] bool isEmpty() const { return width <= 0 || height <= 0; } /**< No pixels. */
    [[nodiscard]] std::int32_t right() const { return x + width; }           /**< x + width. */
    [[nodiscard]] std::int32_t bottom() const { return y + height; }         /**< y + height. */
    /** @brief Number of pixels (0 when empty). */
    [[nodiscard]] std::int64_t area() const {
        return isEmpty() ? 0 : static_cast<std::int64_t>(width) * static_cast<std::int64_t>(height);
    }

    /**
     * @brief The overlap of two rectangles; empty (all zero) when they do not overlap.
     */
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

    /**
     * @brief The smallest rectangle containing both; an empty rectangle is ignored.
     */
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

    /**
     * @brief Whether the two rectangles share at least one pixel.
     */
    [[nodiscard]] bool intersects(const IntRect& other) const { return !intersected(other).isEmpty(); }

    /**
     * @brief Whether @p other lies entirely inside this rectangle.
     */
    [[nodiscard]] bool contains(const IntRect& other) const {
        return other.x >= x && other.y >= y && other.right() <= right() && other.bottom() <= bottom();
    }
};

} // namespace sg
