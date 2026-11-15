#pragma once

#include "SceneNode.h"
#include "Transform2D.h"

namespace sg {

/**
 * Applies a 2D affine transform to its children: their world transform is the parent's times this matrix.
 *
 * Changing the matrix recomputes the world state of this subtree only; the renderer then updates one 48-byte slot
 * per geometry node below and keeps every vertex and batch as is.
 */
class TransformNode : public Node {
public:
    /// A transform node with the given matrix (identity by default).
    explicit TransformNode(const Transform2D& matrix = {}) : Node(Type::Transform), m_matrix(matrix) {}

    [[nodiscard]] const Transform2D& matrix() const { return m_matrix; }
    /// Sets the local matrix; no effect (and no redraw) if it is unchanged.
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
