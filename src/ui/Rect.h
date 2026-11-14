#pragma once

/// @file
/// An axis-aligned rectangle in integer coordinates.

#include "Size.h"


/// An axis-aligned rectangle given by its top-left corner and size.
class Rect {
public:
    /// A rectangle at (@p _x, @p _y) of size @p _w x @p _h.
    /// @param _x Left edge.
    /// @param _y Top edge.
    /// @param _w Width.
    /// @param _h Height.
    Rect(int _x, int _y, int _w, int _h) : m_x(_x), m_y(_y), m_w(_w), m_h(_h) {}

    /// Left edge.
    [[nodiscard]] int x() const { return m_x; }
    /// Top edge.
    [[nodiscard]] int y() const { return m_y; }
    /// Width.
    [[nodiscard]] int width() const { return m_w; }
    /// Height.
    [[nodiscard]] int height() const { return m_h; }

    /// Moves the left edge, keeping the size.
    /// @param _x New left edge.
    void setX(int _x) { m_x = _x; }
    /// Moves the top edge, keeping the size.
    /// @param _y New top edge.
    void setY(int _y) { m_y = _y; }
    /// Sets the width.
    /// @param _w New width.
    void setWidth(int _w) { m_w = _w; }
    /// Sets the height.
    /// @param _h New height.
    void setHeight(int _h) { m_h = _h; }

    /// Sets width and height, keeping the top-left corner.
    /// @param _size New size.
    void resize(const Size& _size) {
        m_w = _size.width();
        m_h = _size.height();
    }
private:
    int m_x, m_y, m_w, m_h;
};
