#pragma once


/// An RGBA colour with integer channels in 0-255.
class Color {
public:
    /// Opaque black.
    Color() : m_r(0), m_g(0), m_b(0), m_a(255) {}

    /**
     * A colour from channel values in 0-255. Values are not clamped.
     *
     * @param _a Alpha (255 = opaque).
     */
    Color(int _r, int _g, int _b, int _a = 255) : m_r(_r), m_g(_g), m_b(_b), m_a(_a) {}

    /// Red channel, 0-255.
    [[nodiscard]] int red() const { return m_r; }
    /// Green channel, 0-255.
    [[nodiscard]] int green() const { return m_g; }
    /// Blue channel, 0-255.
    [[nodiscard]] int blue() const { return m_b; }
    /// Alpha channel, 0-255.
    [[nodiscard]] int alpha() const { return m_a; }

    /// Red channel normalized to 0-1.
    [[nodiscard]] float redF() const { return m_r / 255.0f; }
    /// Green channel normalized to 0-1.
    [[nodiscard]] float greenF() const { return m_g / 255.0f; }
    /// Blue channel normalized to 0-1.
    [[nodiscard]] float blueF() const { return m_b / 255.0f; }
    /// Alpha channel normalized to 0-1.
    [[nodiscard]] float alphaF() const { return m_a / 255.0f; }
private:
    int m_r, m_g, m_b, m_a;
};
