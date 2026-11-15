#pragma once

#include <vector>

#include "RectF.h"
#include "SceneNode.h"

namespace sg {

class ClipNode;

/**
 * The clip in effect below a ClipNode, computed by the update pass.
 *
 * Axis-aligned clips (the common case) reduce to `scissor`. Clips under a rotation or shear also add their node to
 * `stencil_clips`: the renderer draws their shapes into the stencil buffer and tests against it, while `scissor` holds
 * the bounding box of every clip in the chain.
 */
struct ClipContext {
    RectF scissor = RectF::infinite();          /**< Pixels of the render target that may be drawn. */
    std::vector<const ClipNode*> stencil_clips; /**< Non-axis-aligned clips of the chain, outermost first. */
};

/**
 * Clips its children to a rectangle in its own coordinates (intersected with any enclosing clip).
 *
 * The clip node itself does not draw. Its rectangle is mapped through its world transform; under rotation or shear
 * the clip is exact, using the stencil buffer.
 */
class ClipNode : public Node {
public:
    /// A clip node with the given rectangle, in local coordinates.
    explicit ClipNode(const RectF& rect = {}) : Node(Type::Clip), m_rect(rect) {}

    /// The clip rectangle, in local coordinates.
    [[nodiscard]] const RectF& rect() const { return m_rect; }
    /// Sets the clip rectangle; no effect if it is unchanged.
    void setRect(const RectF& rect) {
        if (rect != m_rect) {
            m_rect = rect;
            markDirty(DirtyFlags::Clip);
        }
    }
    /// The clip in effect for the children (valid after an update pass).
    [[nodiscard]] const ClipContext& context() const { return m_context; }
    /// Whether this clip needs the stencil buffer (it is rotated or sheared; valid after an update pass).
    [[nodiscard]] bool usesStencil() const { return m_uses_stencil; }
private:
    friend struct UpdatePassAccess;
    RectF m_rect;
    ClipContext m_context;
    bool m_uses_stencil = false;
};

} // namespace sg
