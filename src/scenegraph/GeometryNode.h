#pragma once

#include <memory>

#include "Geometry.h"
#include "Material.h"
#include "SceneNode.h"

namespace sg {

class GeometryNode : public Node {
public:
    GeometryNode() : Node(Type::Geometry) {}

    GeometryNode(Geometry geometry, Material material)
        : Node(Type::Geometry), m_geometry(std::move(geometry)), m_material(std::move(material)) {}

    [[nodiscard]] const Geometry& geometry() const { return m_geometry; }

    void setGeometry(Geometry geometry) {
        m_geometry = std::move(geometry);
        markDirty(DirtyFlags::Geometry);
    }

    [[nodiscard]] const Material& material() const { return m_material; }

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

class RectangleNode : public GeometryNode {
public:
    explicit RectangleNode(const RectF& rect = {}, ColorF color = ColorF::white())
        : GeometryNode(Geometry::rectangle(rect), Material::flatColor(color)), m_rect(rect) {}

    [[nodiscard]] const RectF& rect() const { return m_rect; }

    void setRect(const RectF& rect) {
        if (rect != m_rect) {
            m_rect = rect;
            setGeometry(Geometry::rectangle(rect));
        }
    }
    [[nodiscard]] ColorF color() const { return material().color; }

    void setColor(ColorF color) { setMaterial(Material::flatColor(color)); }
private:
    RectF m_rect;
};

class ImageNode : public GeometryNode {
public:
    ImageNode(const RectF& rect, std::shared_ptr<Texture> texture, bool smooth = true)
        : GeometryNode(Geometry::rectangle(rect), Material::textured(std::move(texture), smooth)), m_rect(rect) {}

    [[nodiscard]] const RectF& rect() const { return m_rect; }

    void setRect(const RectF& rect) {
        if (rect != m_rect) {
            m_rect = rect;
            setGeometry(Geometry::rectangle(rect));
        }
    }

    void setTexture(std::shared_ptr<Texture> texture, bool smooth = true) {
        setMaterial(Material::textured(std::move(texture), smooth, material().color));
    }
private:
    RectF m_rect;
};

} // namespace sg
