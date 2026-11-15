#pragma once

#include "SceneNode.h"

namespace sg {

/**
 * Renders its subtree once into its own texture and draws that texture as a single quad.
 *
 * The texture is re-rendered only when something inside the subtree changes. Moving, transforming, fading or
 * clipping the layer itself (from above) only changes the quad, so a complex but static subtree costs one draw.
 * Opacity applied above a layer fades the group as one image.
 *
 * The texture covers the bounding box of the subtree at the device pixel ratio, so it is crisp when the layer is
 * only translated; scaling it up blurs it.
 */
class LayerNode : public Node {
public:
    LayerNode() : Node(Type::Layer) {}
};

} // namespace sg
