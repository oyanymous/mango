#pragma once

#include <algorithm>

#include "SceneNode.h"

namespace sg {

/**
 * Multiplies the opacity of everything below it.
 *
 * Each geometry node is faded on its own (overlapping children show through each other); put a LayerNode below
 * to fade a group as one image. A subtree with opacity 0 is not drawn at all.
 */
class OpacityNode : public Node {
public:
    /// An opacity node; @p opacity is clamped to [0, 1].
    explicit OpacityNode(float opacity = 1.0f) : Node(Type::Opacity), m_opacity(std::clamp(opacity, 0.0f, 1.0f)) {}

    /// The local opacity, in [0, 1].
    [[nodiscard]] float opacity() const { return m_opacity; }
    /// Sets the local opacity, clamped to [0, 1]; no effect if it is unchanged.
    void setOpacity(float opacity) {
        opacity = std::clamp(opacity, 0.0f, 1.0f);
        if (opacity != m_opacity) {
            m_opacity = opacity;
            markDirty(DirtyFlags::Opacity);
        }
    }
private:
    float m_opacity;
};

} // namespace sg
