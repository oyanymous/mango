#pragma once

/**
 * @file
 * @brief A 2D size in integer units.
 */


/**
 * @brief A width and height in integer units.
 */
class Size {
public:
    /**
     * @brief A size of @p w x @p h.
     *
     * @param w Width.
     * @param h Height.
     */
    Size(int w, int h) : m_w(w), m_h(h) {}

    /**
     * @brief Width.
     */
    [[nodiscard]] int width() const { return m_w; }
    /**
     * @brief Height.
     */
    [[nodiscard]] int height() const { return m_h; }

    /**
     * @brief Sets the width.
     *
     * @param w New width.
     */
    void setWidth(int w) { m_w = w; }
    /**
     * @brief Sets the height.
     *
     * @param h New height.
     */
    void setHeight(int h) { m_h = h; }
private:
    int m_w, m_h;
};
