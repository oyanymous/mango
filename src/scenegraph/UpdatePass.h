#pragma once

/**
 * @file
 * @brief sg::UpdatePass: walks the dirty parts of a tree, recomputes world state and reports what changed.
 */

#include <cstdint>

#include "SceneNode.h"

namespace sg {

class ClipNode;
class GeometryNode;
class LayerNode;

/**
 * @brief Receives what an UpdatePass found; the Renderer implements it. Every callback has an empty default.
 *
 * @details
 * Called during the traversal, parents before children; the node's world state is already updated. Do not change
 * the tree from a callback.
 */
class UpdateVisitor {
public:
    virtual ~UpdateVisitor() = default;
    /**
     * @brief A geometry node needs its GPU data refreshed.
     *
     * @param node The node.
     * @param flags Its own flags (Geometry, Material, MaterialType, NodeAdded).
     * @param inherited_changed Its world transform, opacity or clip may have changed.
     */
    virtual void geometryUpdated(GeometryNode& node, DirtyFlags flags, bool inherited_changed) {}
    /**
     * @brief A clip node's ClipContext was recomputed.
     */
    virtual void clipUpdated(ClipNode& node) {}
    /**
     * @brief A layer node's own world state (the quad it is drawn as) may have changed.
     *
     * @param node The layer.
     * @param depth Number of LayerNode ancestors (0 for a layer drawn into the window canvas).
     */
    virtual void layerUpdated(LayerNode& node, int depth) {}
    /**
     * @brief Nodes were added to or removed from a render target's subtree, so its paint order changed.
     *
     * @param layer The layer whose content changed, or null for the window canvas.
     */
    virtual void structureChanged(LayerNode* layer) {}
};

/**
 * @brief Counters from one UpdatePass::run.
 */
struct UpdateStats {
    std::uint32_t nodes_visited = 0; /**< Nodes whose flags were looked at, the root included. */
};

/**
 * @brief The update traversal.
 *
 * @details
 * Visits a node only if it or something below it is dirty, and clears the flags it visits. A node whose own matrix,
 * opacity or clip changed, or that was just added, recomputes the world state of its whole subtree (except below a
 * LayerNode, whose content lives in the layer's own coordinates and is unaffected by the layer moving). Clean
 * subtrees are skipped whole, so an unchanged tree costs one visit: the root.
 */
class UpdatePass {
public:
    /**
     * @brief Updates @p root.
     *
     * @param root The tree.
     * @param device_pixel_ratio Pixels per logical unit; the root's world transform scales by it.
     * @param force_all Recompute everything (after the pixel ratio changed, or for a new renderer).
     * @param visitor Told about changes; may be null.
     * @return Counters.
     */
    static UpdateStats run(RootNode& root, float device_pixel_ratio, bool force_all, UpdateVisitor* visitor);
};

} // namespace sg
