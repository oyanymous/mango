#pragma once

#include <memory>

#include "ColorF.h"
#include "SceneTexture.h"

namespace sg {

struct Material {
    ColorF color = ColorF::white();
    std::shared_ptr<Texture> texture;
    bool smooth = true;

    static Material flatColor(ColorF color) { return {color, nullptr, true}; }

    static Material vertexColor() { return {}; }

    static Material textured(std::shared_ptr<Texture> texture, bool smooth = true, ColorF tint = ColorF::white()) {
        return {tint, std::move(texture), smooth};
    }

    [[nodiscard]] bool batchesWith(const Material& other) const {
        return texture == other.texture && (!texture || smooth == other.smooth);
    }
};

} // namespace sg
