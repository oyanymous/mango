#pragma once

/**
 * @file
 * @brief A polygon as a list of points.
 */

#include "Point.h"
#include <vector>


/**
 * @brief A polygon given by its vertices in order.
 */
class Polygon {
public:
    /**
     * @brief The vertex list type.
     */
    using Points = std::vector<Point>;

    /**
     * @brief An empty polygon.
     */
    Polygon() = default;

    /**
     * @brief The vertices, in order.
     */
    [[nodiscard]] const Points& points() const { return m_points; }
private:
    std::vector<Point> m_points;
};
