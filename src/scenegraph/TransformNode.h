#pragma once

#include "SceneNode.h"
#include "Transform2D.h"

namespace sg {

class TransformNode : public Node {
public:
    explicit TransformNode(const Transform2D& matrix = {}) : Node(Type::Transform), m_matrix(matrix) {}

    [[nodiscard]] const Transform2D& matrix() const { return m_matrix; }

    void setMatrix(const Transform2D& matrix) {
        if (matrix != m_matrix) {
            m_matrix = matrix;
            markDirty(DirtyFlags::Matrix);
        }
    }
private:
    Transform2D m_matrix;
};

} // namespace sg
