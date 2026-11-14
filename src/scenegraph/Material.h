#pragma once

/**
 * @file
 * @brief sg::Material: how a GeometryNode is shaded.
 */

#include <memory>

#include "ColorF.h"
#include "SceneTexture.h"

namespace sg {

/**
 * @brief The shading of a GeometryNode: a colour, optionally times a texture.
 *
 * @details
 * The fragment colour is `texture (if any) * vertex colour * color * opacity`. Materials are values: give a node a
 * new one with GeometryNode::setMaterial. Changing only `color` updates one GPU slot; changing the texture or
 * `smooth` (or adding/removing a texture) selects another pipeline or bind group, which rebuilds batches.
 */
struct Material {
    ColorF color = ColorF::white();   /**< Colour multiplied into every fragment. */
    std::shared_ptr<Texture> texture; /**< Optional texture, sampled with the vertices' texture coordinates. */
    bool smooth = true;               /**< Linear filtering of the texture; false for nearest (pixel art). */

    /** @brief A flat colour (with white vertices, the colour as is). */
    static Material flatColor(ColorF color) { return {color, nullptr, true}; }
    /** @brief Vertex colours only. */
    static Material vertexColor() { return {}; }
    /** @brief A texture, optionally tinted. */
    static Material textured(std::shared_ptr<Texture> texture, bool smooth = true, ColorF tint = ColorF::white()) {
        return {tint, std::move(texture), smooth};
    }

    /**
     * @brief Whether two materials draw with the same pipeline and bind group, i.e. can share a batch.
     */
    [[nodiscard]] bool batchesWith(const Material& other) const {
        return texture == other.texture && (!texture || smooth == other.smooth);
    }
};

} // namespace sg
