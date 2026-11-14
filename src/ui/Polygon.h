#pragma once

/// @file
/// A polygon as a list of points.

#include "Point.h"
#include <vector>


/// A polygon given by its vertices in order.
class Polygon {
public:
    /// The vertex list type.
    using Points = std::vector<Point>;

    /// An empty polygon.
    Polygon() = default;

    /// The vertices, in order.
    [[nodiscard]] const Points& points() const { return m_points; }
private:
    std::vector<Point> m_points;
};
