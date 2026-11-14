/**
 * @file
 * @brief sg::Geometry: bounds, list conversion and shape helpers.
 */

#include "Geometry.h"

#include <algorithm>
#include <cmath>

namespace sg {

Geometry::Geometry(DrawMode mode, std::vector<Vertex> vertices, std::vector<std::uint32_t> indices)
    : m_mode(mode), m_vertices(std::move(vertices)), m_indices(std::move(indices)) {
    if (m_vertices.empty()) {
        return;
    }
    float x0 = m_vertices[0].x;
    float y0 = m_vertices[0].y;
    float x1 = x0;
    float y1 = y0;
    for (const Vertex& vertex : m_vertices) {
        x0 = std::min(x0, vertex.x);
        y0 = std::min(y0, vertex.y);
        x1 = std::max(x1, vertex.x);
        y1 = std::max(y1, vertex.y);
    }
    // Lines along an axis have no area; give them a hairline so they still produce damage and draw.
    if (isLines()) {
        x1 = std::max(x1, x0 + 1.0f);
        y1 = std::max(y1, y0 + 1.0f);
    }
    m_bounds = {x0, y0, x1 - x0, y1 - y0};
}

Geometry Geometry::rectangle(const RectF& rect, ColorF color) {
    const std::uint32_t rgba = packRgba8(color);
    return Geometry(DrawMode::Triangles,
                    {
                        {rect.x, rect.y, 0.0f, 0.0f, rgba},
                        {rect.right(), rect.y, 1.0f, 0.0f, rgba},
                        {rect.right(), rect.bottom(), 1.0f, 1.0f, rgba},
                        {rect.x, rect.bottom(), 0.0f, 1.0f, rgba},
                    },
                    {0, 1, 2, 2, 3, 0});
}

Geometry Geometry::convexPolygon(std::span<const PointF> points, ColorF color) {
    const std::uint32_t rgba = packRgba8(color);
    std::vector<Vertex> vertices;
    vertices.reserve(points.size());
    for (PointF p : points) {
        vertices.push_back({p.x, p.y, 0.0f, 0.0f, rgba});
    }
    if (vertices.size() < 3) {
        return Geometry(DrawMode::Triangles, {});
    }
    return Geometry(DrawMode::TriangleFan, std::move(vertices));
}

Geometry Geometry::polyline(std::span<const PointF> points, float width, bool closed, ColorF color) {
    const std::uint32_t rgba = packRgba8(color);
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    const std::size_t segments = points.size() < 2 ? 0 : points.size() - (closed ? 0 : 1);
    const float half = width * 0.5f;
    for (std::size_t i = 0; i < segments; ++i) {
        const PointF a = points[i];
        const PointF b = points[(i + 1) % points.size()];
        const float dx = b.x - a.x;
        const float dy = b.y - a.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (length <= 0.0f) {
            continue;
        }
        // Normal and direction scaled to half the width; extending along the direction squares the joints.
        const float nx = -dy / length * half;
        const float ny = dx / length * half;
        const float ex = dx / length * half;
        const float ey = dy / length * half;
        const auto base = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back({a.x - ex + nx, a.y - ey + ny, 0.0f, 0.0f, rgba});
        vertices.push_back({b.x + ex + nx, b.y + ey + ny, 1.0f, 0.0f, rgba});
        vertices.push_back({b.x + ex - nx, b.y + ey - ny, 1.0f, 1.0f, rgba});
        vertices.push_back({a.x - ex - nx, a.y - ey - ny, 0.0f, 1.0f, rgba});
        indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});
    }
    return Geometry(DrawMode::Triangles, std::move(vertices), std::move(indices));
}

std::vector<std::uint32_t> Geometry::listIndices() const {
    std::vector<std::uint32_t> source = m_indices;
    if (source.empty()) {
        source.resize(m_vertices.size());
        for (std::size_t i = 0; i < source.size(); ++i) {
            source[i] = static_cast<std::uint32_t>(i);
        }
    }
    // Indices past the end would read other nodes' vertices from the shared pool; drop such primitives.
    const auto valid = [this](std::uint32_t index) { return index < m_vertices.size(); };
    std::vector<std::uint32_t> out;
    switch (m_mode) {
    case DrawMode::Triangles:
        for (std::size_t i = 0; i + 2 < source.size(); i += 3) {
            if (valid(source[i]) && valid(source[i + 1]) && valid(source[i + 2])) {
                out.insert(out.end(), {source[i], source[i + 1], source[i + 2]});
            }
        }
        break;
    case DrawMode::TriangleStrip:
        for (std::size_t i = 0; i + 2 < source.size(); ++i) {
            // Every other triangle swaps two vertices to keep a consistent winding.
            const std::uint32_t a = source[i];
            const std::uint32_t b = source[i + (i % 2 == 0 ? 1 : 2)];
            const std::uint32_t c = source[i + (i % 2 == 0 ? 2 : 1)];
            if (valid(a) && valid(b) && valid(c)) {
                out.insert(out.end(), {a, b, c});
            }
        }
        break;
    case DrawMode::TriangleFan:
        for (std::size_t i = 1; i + 1 < source.size(); ++i) {
            if (valid(source[0]) && valid(source[i]) && valid(source[i + 1])) {
                out.insert(out.end(), {source[0], source[i], source[i + 1]});
            }
        }
        break;
    case DrawMode::Lines:
        for (std::size_t i = 0; i + 1 < source.size(); i += 2) {
            if (valid(source[i]) && valid(source[i + 1])) {
                out.insert(out.end(), {source[i], source[i + 1]});
            }
        }
        break;
    case DrawMode::LineStrip:
        for (std::size_t i = 0; i + 1 < source.size(); ++i) {
            if (valid(source[i]) && valid(source[i + 1])) {
                out.insert(out.end(), {source[i], source[i + 1]});
            }
        }
        break;
    }
    return out;
}

} // namespace sg
