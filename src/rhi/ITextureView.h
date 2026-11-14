#pragma once

#include <optional>
#include <string>

#include "ITexture.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

enum class TextureViewDimension {
    Texture2D,
    Texture2DArray,
    Texture3D,
    TextureCube,
};

// Which planes of a depth/stencil texture a view sees. Colour textures always use All.
enum class TextureAspect {
    All,
    DepthOnly,
    StencilOnly,
};

struct TextureViewDesc {
    TextureRef texture;
    // Unset fields follow the texture: same format, and the dimension that matches the texture.
    std::optional<Format> format;
    std::optional<TextureViewDimension> dimension;
    TextureAspect aspect = TextureAspect::All;
    TextureSubresourceRange range;
    std::string label;
};

class ITextureView {
public:
    virtual ~ITextureView() = default;

    virtual TextureRef getTexture() const = 0;
};

} // namespace rhi
