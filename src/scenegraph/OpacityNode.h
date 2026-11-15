#pragma once

#include <algorithm>

#include "SceneNode.h"

namespace sg {

class OpacityNode : public Node {
public:
    explicit OpacityNode(float opacity = 1.0f) : Node(Type::Opacity), m_opacity(std::clamp(opacity, 0.0f, 1.0f)) {}

    [[nodiscard]] float opacity() const { return m_opacity; }

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
