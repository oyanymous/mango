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

enum class TextureAspect {
    All,
    DepthOnly,
    StencilOnly,
};

struct TextureViewDesc {
    TextureRef texture;

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
