#pragma once

namespace sg {

struct PointF {
    float x = 0.0f;
    float y = 0.0f;

    friend bool operator==(const PointF&, const PointF&) = default;
    friend PointF operator+(PointF a, PointF b) { return {a.x + b.x, a.y + b.y}; }
    friend PointF operator-(PointF a, PointF b) { return {a.x - b.x, a.y - b.y}; }
    friend PointF operator*(PointF a, float s) { return {a.x * s, a.y * s}; }
};

} // namespace sg
