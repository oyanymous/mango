#pragma once

/**
 * @file
 * @brief sg::GeometryNode, the node that draws, and its conveniences sg::RectangleNode and sg::ImageNode.
 */

#include <memory>

#include "Geometry.h"
#include "Material.h"
#include "SceneNode.h"

namespace sg {

/**
 * @brief Draws a Geometry with a Material, in its parent's coordinates.
 *
 * @details
 * The only node that produces draw calls. Children are drawn on top of it.
 */
class GeometryNode : public Node {
public:
    GeometryNode() : Node(Type::Geometry) {}
    /**
     * @brief A node drawing @p geometry with @p material.
     */
    GeometryNode(Geometry geometry, Material material)
        : Node(Type::Geometry), m_geometry(std::move(geometry)), m_material(std::move(material)) {}

    /** @brief The geometry. */
    [[nodiscard]] const Geometry& geometry() const { return m_geometry; }
    /**
     * @brief Replaces the geometry. Same vertex and index counts update in place; other sizes rebuild batches.
     */
    void setGeometry(Geometry geometry) {
        m_geometry = std::move(geometry);
        markDirty(DirtyFlags::Geometry);
    }

    /** @brief The material. */
    [[nodiscard]] const Material& material() const { return m_material; }
    /**
     * @brief Replaces the material; see Material for what each change costs. No effect if it is equal.
     */
    void setMaterial(Material material) {
        const bool same_batch = material.batchesWith(m_material);
        if (same_batch && material.color == m_material.color) {
            return;
        }
        m_material = std::move(material);
        markDirty(same_batch ? DirtyFlags::Material : DirtyFlags::MaterialType);
    }
private:
    Geometry m_geometry;
    Material m_material;
};

/**
 * @brief A solid-colour rectangle.
 */
class RectangleNode : public GeometryNode {
public:
    /**
     * @brief A rectangle at @p rect (local coordinates) filled with @p color.
     */
    explicit RectangleNode(const RectF& rect = {}, ColorF color = ColorF::white())
        : GeometryNode(Geometry::rectangle(rect), Material::flatColor(color)), m_rect(rect) {}

    /** @brief The rectangle. */
    [[nodiscard]] const RectF& rect() const { return m_rect; }
    /** @brief Moves or resizes the rectangle (updates four vertices in place). */
    void setRect(const RectF& rect) {
        if (rect != m_rect) {
            m_rect = rect;
            setGeometry(Geometry::rectangle(rect));
        }
    }
    /** @brief The fill colour. */
    [[nodiscard]] ColorF color() const { return material().color; }
    /** @brief Changes the fill colour (updates one GPU slot). */
    void setColor(ColorF color) { setMaterial(Material::flatColor(color)); }
private:
    RectF m_rect;
};

/**
 * @brief A rectangle showing a texture, stretched to fit.
 */
class ImageNode : public GeometryNode {
public:
    /**
     * @brief An image of @p texture at @p rect (local coordinates).
     */
    ImageNode(const RectF& rect, std::shared_ptr<Texture> texture, bool smooth = true)
        : GeometryNode(Geometry::rectangle(rect), Material::textured(std::move(texture), smooth)), m_rect(rect) {}

    /** @brief The rectangle. */
    [[nodiscard]] const RectF& rect() const { return m_rect; }
    /** @brief Moves or resizes the image (updates four vertices in place). */
    void setRect(const RectF& rect) {
        if (rect != m_rect) {
            m_rect = rect;
            setGeometry(Geometry::rectangle(rect));
        }
    }
    /** @brief Shows another texture (rebuilds batches). */
    void setTexture(std::shared_ptr<Texture> texture, bool smooth = true) {
        setMaterial(Material::textured(std::move(texture), smooth, material().color));
    }
private:
    RectF m_rect;
};

} // namespace sg
