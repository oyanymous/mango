#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "ColorF.h"
#include "PointF.h"
#include "RectF.h"

namespace sg {

struct Vertex {
    float x = 0.0f;
    float y = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    std::uint32_t rgba = 0xffffffffu;

    friend bool operator==(const Vertex&, const Vertex&) = default;
};

enum class DrawMode {
    Triangles,
    TriangleStrip,
    TriangleFan,
    Lines,
    LineStrip,
};

class Geometry {
public:
    Geometry() = default;

    Geometry(DrawMode mode, std::vector<Vertex> vertices, std::vector<std::uint32_t> indices = {});

    static Geometry rectangle(const RectF& rect, ColorF color = ColorF::white());

    static Geometry convexPolygon(std::span<const PointF> points, ColorF color = ColorF::white());

    static Geometry polyline(std::span<const PointF> points, float width, bool closed = false,
                             ColorF color = ColorF::white());

    [[nodiscard]] DrawMode mode() const { return m_mode; }
    [[nodiscard]] const std::vector<Vertex>& vertices() const { return m_vertices; }

    [[nodiscard]] const std::vector<std::uint32_t>& indices() const { return m_indices; }

    [[nodiscard]] const RectF& bounds() const { return m_bounds; }

    [[nodiscard]] bool isLines() const { return m_mode == DrawMode::Lines || m_mode == DrawMode::LineStrip; }

    [[nodiscard]] std::vector<std::uint32_t> listIndices() const;
private:
    DrawMode m_mode = DrawMode::Triangles;
    std::vector<Vertex> m_vertices;
    std::vector<std::uint32_t> m_indices;
    RectF m_bounds;
};

} // namespace sg
