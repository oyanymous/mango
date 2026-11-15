#pragma once

#include <cstdint>

#include "SceneNode.h"

namespace sg {

class ClipNode;
class GeometryNode;
class LayerNode;

/**
 * Receives what an UpdatePass found; the Renderer implements it. Every callback has an empty default.
 *
 * Called during the traversal, parents before children; the node's world state is already updated. Do not change
 * the tree from a callback.
 */
class UpdateVisitor {
public:
    virtual ~UpdateVisitor() = default;
    /**
     * A geometry node needs its GPU data refreshed.
     *
     * @param flags Its own flags (Geometry, Material, MaterialType, NodeAdded).
     * @param inherited_changed Its world transform, opacity or clip may have changed.
     */
    virtual void geometryUpdated(GeometryNode& node, DirtyFlags flags, bool inherited_changed) {}
    /// A clip node's ClipContext was recomputed.
    virtual void clipUpdated(ClipNode& node) {}
    /**
     * A layer node's own world state (the quad it is drawn as) may have changed.
     *
     * @param depth Number of LayerNode ancestors (0 for a layer drawn into the window canvas).
     */
    virtual void layerUpdated(LayerNode& node, int depth) {}
    /**
     * Nodes were added to or removed from a render target's subtree, so its paint order changed.
     *
     * @param layer The layer whose content changed, or null for the window canvas.
     */
    virtual void structureChanged(LayerNode* layer) {}
};

/// Counters from one UpdatePass::run.
struct UpdateStats {
    std::uint32_t nodes_visited = 0; /**< Nodes whose flags were looked at, the root included. */
};

/**
 * The update traversal.
 *
 * Visits a node only if it or something below it is dirty, and clears the flags it visits. A node whose own matrix,
 * opacity or clip changed, or that was just added, recomputes the world state of its whole subtree (except below a
 * LayerNode, whose content lives in the layer's own coordinates and is unaffected by the layer moving). Clean
 * subtrees are skipped whole, so an unchanged tree costs one visit: the root.
 */
class UpdatePass {
public:
    /**
     * Updates @p root.
     *
     * @param device_pixel_ratio Pixels per logical unit; the root's world transform scales by it.
     * @param force_all Recompute everything (after the pixel ratio changed, or for a new renderer).
     * @param visitor Told about changes; may be null.
     */
    static UpdateStats run(RootNode& root, float device_pixel_ratio, bool force_all, UpdateVisitor* visitor);
};

} // namespace sg
