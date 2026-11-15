#pragma once

#include "Point.h"
#include <vector>


/// A polygon given by its vertices in order.
class Polygon {
public:
    using Points = std::vector<Point>;

    Polygon() = default;

    /// The vertices, in order.
    [[nodiscard]] const Points& points() const { return m_points; }
private:
    std::vector<Point> m_points;
};
