#pragma once

/**
 * @file
 * @brief A line segment in integer coordinates.
 */

#include "Point.h"


/**
 * @brief A line segment between two integer points.
 */
class Line {
public:
    /**
     * @brief A segment from (@p x0, @p y0) to (@p x1, @p y1).
     *
     * @param x0 Start x.
     * @param y0 Start y.
     * @param x1 End x.
     * @param y1 End y.
     */
    Line(int x0, int y0, int x1, int y1) : p0(x0, y0), p1(x1, y1) {}
private:
    Point p0, p1;
};
