#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "RectF.h"

namespace sg {

class DamageRegion {
public:
    explicit DamageRegion(std::size_t max_rects = 8, double full_redraw_fraction = 0.5)
        : m_max_rects(max_rects), m_full_fraction(full_redraw_fraction) {}

    void setBounds(const IntRect& bounds) {
        m_bounds = bounds;
        clear();
    }
    [[nodiscard]] const IntRect& bounds() const { return m_bounds; }

    void add(const IntRect& rect);

    void addAll();
    void clear() {
        m_rects.clear();
        m_full = false;
    }

    [[nodiscard]] bool isEmpty() const { return m_rects.empty(); }

    [[nodiscard]] bool isFull() const { return m_full; }

    [[nodiscard]] const std::vector<IntRect>& rects() const { return m_rects; }

    [[nodiscard]] std::int64_t area() const;
private:
    std::vector<IntRect> m_rects;
    IntRect m_bounds;
    std::size_t m_max_rects;
    double m_full_fraction;
    bool m_full = false;
};

} // namespace sg
