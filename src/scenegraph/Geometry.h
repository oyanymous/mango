#pragma once

/**
 * @file
 * @brief sg::Geometry: CPU-side vertex and index data of a GeometryNode, with helpers for common shapes.
 */

#include <cstdint>
#include <span>
#include <vector>

#include "ColorF.h"
#include "PointF.h"
#include "RectF.h"

namespace sg {

/**
 * @brief One vertex: position in local coordinates, texture coordinate and a colour.
 *
 * @details
 * The colour multiplies the material's colour (and texture), so white vertices show the material as is.
 */
struct Vertex {
    float x = 0.0f;                   /**< Position, x. */
    float y = 0.0f;                   /**< Position, y. */
    float u = 0.0f;                   /**< Texture coordinate, u (0 = left edge). */
    float v = 0.0f;                   /**< Texture coordinate, v (0 = top edge). */
    std::uint32_t rgba = 0xffffffffu; /**< Straight-alpha colour from packRgba8() (white by default). */

    /** @brief Member-wise equality. */

    friend bool operator==(const Vertex&, const Vertex&) = default;
};

/**
 * @brief How vertices (or indices) form primitives.
 */
enum class DrawMode {
    Triangles,     /**< Every three form a triangle. */
    TriangleStrip, /**< Each one after the first two forms a triangle with the previous two. */
    TriangleFan,   /**< Each one after the first two forms a triangle with the previous one and the first. */
    Lines,         /**< Every two form a one-pixel line. */
    LineStrip,     /**< Consecutive ones form a connected one-pixel line. */
};

/**
 * @brief Vertices, optional indices and a draw mode, plus their bounding box.
 *
 * @details
 * Strips and fans are converted to lists by the renderer, so geometry of every mode batches with other geometry of
 * the same material: triangle modes with triangles, line modes with lines.
 */
class Geometry {
public:
    Geometry() = default;
    /**
     * @brief Geometry from vertices and, optionally, indices into them.
     *
     * @param mode How the vertices (or the indices, when given) form primitives.
     * @param vertices The vertices.
     * @param indices Indices into @p vertices; empty to use the vertices in order.
     */
    Geometry(DrawMode mode, std::vector<Vertex> vertices, std::vector<std::uint32_t> indices = {});

    /**
     * @brief Two triangles covering @p rect, with texture coordinates from (0, 0) to (1, 1).
     */
    static Geometry rectangle(const RectF& rect, ColorF color = ColorF::white());
    /**
     * @brief A filled convex polygon (a triangle fan from the first point). Fewer than 3 points give no triangles.
     */
    static Geometry convexPolygon(std::span<const PointF> points, ColorF color = ColorF::white());
    /**
     * @brief A thick polyline: one quad per segment, @p width wide, centred on the line, with square joints.
     *
     * @param points The line's points; fewer than 2 give no triangles.
     * @param width Line width in local units.
     * @param closed Whether to add a segment from the last point back to the first.
     * @param color Vertex colour.
     */
    static Geometry polyline(std::span<const PointF> points, float width, bool closed = false,
                             ColorF color = ColorF::white());

    /** @brief The draw mode. */
    [[nodiscard]] DrawMode mode() const { return m_mode; }
    /** @brief The vertices. */
    [[nodiscard]] const std::vector<Vertex>& vertices() const { return m_vertices; }
    /** @brief The indices; empty means the vertices are used in order. */
    [[nodiscard]] const std::vector<std::uint32_t>& indices() const { return m_indices; }
    /** @brief The bounding box of the vertices, in local coordinates. */
    [[nodiscard]] const RectF& bounds() const { return m_bounds; }
    /** @brief Whether the primitives are lines (DrawMode::Lines or LineStrip). */
    [[nodiscard]] bool isLines() const { return m_mode == DrawMode::Lines || m_mode == DrawMode::LineStrip; }

    /**
     * @brief The primitives as a triangle list or line list: indices into vertices(), strips and fans expanded.
     */
    [[nodiscard]] std::vector<std::uint32_t> listIndices() const;
private:
    DrawMode m_mode = DrawMode::Triangles;
    std::vector<Vertex> m_vertices;
    std::vector<std::uint32_t> m_indices;
    RectF m_bounds;
};

} // namespace sg
