#pragma once

/// @file
/// A 2D size in integer units.


/// A width and height in integer units.
class Size {
public:
    /// A size of @p w x @p h.
    /// @param w Width.
    /// @param h Height.
    Size(int w, int h) : m_w(w), m_h(h) {}

    /// Width.
    [[nodiscard]] int width() const { return m_w; }
    /// Height.
    [[nodiscard]] int height() const { return m_h; }

    /// Sets the width.
    /// @param w New width.
    void setWidth(int w) { m_w = w; }
    /// Sets the height.
    /// @param h New height.
    void setHeight(int h) { m_h = h; }
private:
    int m_w, m_h;
};
