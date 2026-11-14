/**
 * @file
 * @brief sg::UpdatePass.
 */

#include "UpdatePass.h"

#include "ClipNode.h"
#include "GeometryNode.h"
#include "LayerNode.h"
#include "OpacityNode.h"
#include "TransformNode.h"

namespace sg {

namespace {

/** @brief The traversal's state for one subtree. */
struct Traversal {
    float device_pixel_ratio = 1.0f;
    UpdateVisitor* visitor = nullptr;
    UpdateStats stats;
};

/** @brief World state passed from a parent to its children. */
struct Inherited {
    Transform2D world;
    float opacity = 1.0f;
    const ClipContext* clip = nullptr;
    LayerNode* layer = nullptr;
    int layer_depth = 0;
};

} // namespace

/**
 * @brief The traversal itself; a friend of Node and ClipNode, so it can write their world state.
 */
struct UpdatePassAccess {
    /**
     * @brief Visits @p node: refreshes its world state when needed, reports it, then recurses into dirty children.
     *
     * @details
     * `inherited_changed` means the parent's world state changed, so this node's must be recomputed; `is_new` means
     * the node sits in a subtree just added to the tree (so even content below layers must be computed).
     */
    static void visit(Node& node, const Inherited& in, bool inherited_changed, bool is_new, Traversal& t) {
        ++t.stats.nodes_visited;
        const DirtyFlags flags = node.m_dirty;
        node.m_dirty = DirtyFlags::None;
        is_new = is_new || hasAny(flags, DirtyFlags::NodeAdded);
        const bool changed =
            inherited_changed || is_new || hasAny(flags, DirtyFlags::Matrix | DirtyFlags::Opacity | DirtyFlags::Clip);

        if (t.visitor != nullptr) {
            // An added node joins its parent's target; a removed child left this node's (a layer's own target).
            if (hasAny(flags, DirtyFlags::NodeAdded)) {
                t.visitor->structureChanged(in.layer);
            }
            if (hasAny(flags, DirtyFlags::NodeRemoved)) {
                t.visitor->structureChanged(node.type() == Node::Type::Layer ? static_cast<LayerNode*>(&node)
                                                                             : in.layer);
            }
        }
        if (changed) {
            node.m_world = in.world;
            node.m_world_opacity = in.opacity;
            node.m_clip = in.clip;
            node.m_layer = in.layer;
        }

        Inherited out{node.m_world, node.m_world_opacity, node.m_clip, node.m_layer, in.layer_depth};
        bool children_changed = changed;
        switch (node.type()) {
        case Node::Type::Transform:
            out.world = node.m_world * static_cast<TransformNode&>(node).matrix();
            break;
        case Node::Type::Opacity:
            out.opacity = node.m_world_opacity * static_cast<OpacityNode&>(node).opacity();
            break;
        case Node::Type::Clip: {
            auto& clip = static_cast<ClipNode&>(node);
            if (changed) {
                const ClipContext* parent = node.m_clip;
                clip.m_context.stencil_clips =
                    parent != nullptr ? parent->stencil_clips : std::vector<const ClipNode*>{};
                const RectF parent_scissor = parent != nullptr ? parent->scissor : RectF::infinite();
                clip.m_context.scissor = parent_scissor.intersected(node.m_world.mapRect(clip.rect()));
                clip.m_uses_stencil = !node.m_world.isAxisAligned();
                if (clip.m_uses_stencil) {
                    clip.m_context.stencil_clips.push_back(&clip);
                }
                if (t.visitor != nullptr) {
                    t.visitor->clipUpdated(clip);
                }
            }
            out.clip = &clip.m_context;
            break;
        }
        case Node::Type::Geometry:
            if (t.visitor != nullptr &&
                (changed || hasAny(flags, DirtyFlags::Geometry | DirtyFlags::Material | DirtyFlags::MaterialType))) {
                t.visitor->geometryUpdated(static_cast<GeometryNode&>(node), flags, changed);
            }
            break;
        case Node::Type::Layer: {
            auto& layer = static_cast<LayerNode&>(node);
            if (t.visitor != nullptr && changed) {
                t.visitor->layerUpdated(layer, in.layer_depth);
            }
            // The content is drawn in the layer's own pixels: moving the layer does not touch it.
            out = Inherited{Transform2D::scaling(t.device_pixel_ratio, t.device_pixel_ratio), 1.0f, nullptr, &layer,
                            in.layer_depth + 1};
            children_changed = is_new;
            break;
        }
        case Node::Type::Basic:
        case Node::Type::Root:
            break;
        }

        for (const std::unique_ptr<Node>& child : node.m_children) {
            if (children_changed || child->m_dirty != DirtyFlags::None) {
                visit(*child, out, children_changed, is_new, t);
            }
        }
    }
};

UpdateStats UpdatePass::run(RootNode& root, float device_pixel_ratio, bool force_all, UpdateVisitor* visitor) {
    Traversal t{device_pixel_ratio, visitor, {}};
    const Inherited in{Transform2D::scaling(device_pixel_ratio, device_pixel_ratio), 1.0f, nullptr, nullptr, 0};
    // force_all counts as "new": a changed pixel ratio changes layer contents too.
    UpdatePassAccess::visit(root, in, force_all, force_all, t);
    return t.stats;
}

} // namespace sg
