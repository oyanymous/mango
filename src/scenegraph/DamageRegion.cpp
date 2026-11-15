/**
 * @file
 * @brief sg::DamageRegion.
 */

#include "DamageRegion.h"

namespace sg {

namespace {

/// Whether two rectangles overlap or share an edge (merging those never adds undamaged pixels in a line).
bool touches(const IntRect& a, const IntRect& b) {
    return a.x <= b.right() && b.x <= a.right() && a.y <= b.bottom() && b.y <= a.bottom();
}

} // namespace

void DamageRegion::add(const IntRect& rect) {
    if (m_full) {
        return;
    }
    IntRect merged = rect.intersected(m_bounds);
    if (merged.isEmpty()) {
        return;
    }
    // Merge with everything it touches; a merge can make the result touch rectangles it did not touch before.
    for (bool again = true; again;) {
        again = false;
        for (std::size_t i = 0; i < m_rects.size(); ++i) {
            if (touches(m_rects[i], merged)) {
                merged = merged.united(m_rects[i]);
                m_rects.erase(m_rects.begin() + static_cast<std::ptrdiff_t>(i));
                again = true;
                break;
            }
        }
    }
    m_rects.push_back(merged);
    if (m_rects.size() > m_max_rects) {
        IntRect all;
        for (const IntRect& r : m_rects) {
            all = all.united(r);
        }
        m_rects = {all};
    }
    if (static_cast<double>(area()) > m_full_fraction * static_cast<double>(m_bounds.area())) {
        addAll();
    }
}

void DamageRegion::addAll() {
    m_rects.clear();
    if (!m_bounds.isEmpty()) {
        m_rects.push_back(m_bounds);
    }
    m_full = true;
}

std::int64_t DamageRegion::area() const {
    std::int64_t total = 0;
    for (const IntRect& r : m_rects) {
        total += r.area();
    }
    return total;
}

} // namespace sg
