#pragma once

/**
 * @file
 * @brief A 2D point in integer coordinates.
 */


/**
 * @brief A 2D point (or offset) with integer coordinates.
 */
class Point {
public:
    /**
     * @brief The point (100, 100).
     */
    Point() : m_x(100), m_y(100) {}
    /**
     * @brief The point (@p _x, @p _y).
     *
     * @param _x X coordinate.
     * @param _y Y coordinate.
     */
    Point(int _x, int _y) : m_x(_x), m_y(_y) {}

    /**
     * @brief X coordinate.
     */
    [[nodiscard]] int x() const { return m_x; }
    /**
     * @brief Y coordinate.
     */
    [[nodiscard]] int y() const { return m_y; }

    /**
     * @brief Sets the x coordinate.
     *
     * @param _x New x.
     */
    void setX(int _x) { m_x = _x; }
    /**
     * @brief Sets the y coordinate.
     *
     * @param _y New y.
     */
    void setY(int _y) { m_y = _y; }

    /**
     * @brief Component-wise sum.
     *
     * @param other Point to add.
     * @return (x + other.x, y + other.y).
     */
    Point operator+(const Point& other) const { return Point(m_x + other.m_x, m_y + other.m_y); }

    /**
     * @brief Component-wise difference.
     *
     * @param other Point to subtract.
     * @return (x - other.x, y - other.y).
     */
    Point operator-(const Point& other) const { return Point(m_x - other.m_x, m_y - other.m_y); }
private:
    int m_x, m_y;
};
