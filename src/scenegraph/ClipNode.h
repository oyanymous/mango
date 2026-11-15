#pragma once

#include <vector>

#include "RectF.h"
#include "SceneNode.h"

namespace sg {

class ClipNode;

struct ClipContext {
    RectF scissor = RectF::infinite();
    std::vector<const ClipNode*> stencil_clips;
};

class ClipNode : public Node {
public:
    explicit ClipNode(const RectF& rect = {}) : Node(Type::Clip), m_rect(rect) {}

    [[nodiscard]] const RectF& rect() const { return m_rect; }

    void setRect(const RectF& rect) {
        if (rect != m_rect) {
            m_rect = rect;
            markDirty(DirtyFlags::Clip);
        }
    }

    [[nodiscard]] const ClipContext& context() const { return m_context; }

    [[nodiscard]] bool usesStencil() const { return m_uses_stencil; }
private:
    friend struct UpdatePassAccess;
    RectF m_rect;
    ClipContext m_context;
    bool m_uses_stencil = false;
};

} // namespace sg
