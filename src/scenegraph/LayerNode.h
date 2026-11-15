#pragma once

#include "SceneNode.h"

namespace sg {

class LayerNode : public Node {
public:
    LayerNode() : Node(Type::Layer) {}
};

} // namespace sg
