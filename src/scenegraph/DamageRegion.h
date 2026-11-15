#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "RectF.h"

namespace sg {

/**
 * A short list of non-overlapping pixel rectangles to redraw, inside a render target's bounds.
 *
 * Overlapping (or touching) rectangles are merged into their bounding box. When more than `max_rects` remain, or
 * the damaged area exceeds `full_redraw_fraction` of the target, the region becomes the whole target: one big scissor
 * is cheaper than many small ones.
 */
class DamageRegion {
public:
    /**
     * An empty region.
     *
     * @param max_rects Most rectangles kept before collapsing to their bounding box.
     * @param full_redraw_fraction Damaged fraction of the target above which the whole target is redrawn.
     */
    explicit DamageRegion(std::size_t max_rects = 8, double full_redraw_fraction = 0.5)
        : m_max_rects(max_rects), m_full_fraction(full_redraw_fraction) {}

    /// Sets the render target's bounds; rectangles are clipped to them. Clears the region.
    void setBounds(const IntRect& bounds) {
        m_bounds = bounds;
        clear();
    }
    [[nodiscard]] const IntRect& bounds() const { return m_bounds; }

    /// Adds a rectangle (clipped to the bounds; ignored when empty).
    void add(const IntRect& rect);
    /// Damages the whole target.
    void addAll();
    void clear() {
        m_rects.clear();
        m_full = false;
    }

    /// Whether nothing needs redrawing.
    [[nodiscard]] bool isEmpty() const { return m_rects.empty(); }
    /// Whether the whole target needs redrawing.
    [[nodiscard]] bool isFull() const { return m_full; }
    /// The rectangles, non-overlapping, inside bounds().
    [[nodiscard]] const std::vector<IntRect>& rects() const { return m_rects; }
    /// Sum of the rectangles' areas, in pixels.
    [[nodiscard]] std::int64_t area() const;
private:
    std::vector<IntRect> m_rects;
    IntRect m_bounds;
    std::size_t m_max_rects;
    double m_full_fraction;
    bool m_full = false;
};

} // namespace sg
