#pragma once

#include <cstdint>

#include "SceneNode.h"

namespace sg {

class ClipNode;
class GeometryNode;
class LayerNode;

class UpdateVisitor {
public:
    virtual ~UpdateVisitor() = default;

    virtual void geometryUpdated(GeometryNode& node, DirtyFlags flags, bool inherited_changed) {}

    virtual void clipUpdated(ClipNode& node) {}

    virtual void layerUpdated(LayerNode& node, int depth) {}

    virtual void structureChanged(LayerNode* layer) {}
};

struct UpdateStats {
    std::uint32_t nodes_visited = 0;
};

class UpdatePass {
public:
    static UpdateStats run(RootNode& root, float device_pixel_ratio, bool force_all, UpdateVisitor* visitor);
};

} // namespace sg
