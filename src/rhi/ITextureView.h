#pragma once

/// @file
/// rhi::ITextureView: how shaders and render targets see (part of) a texture.

#include <optional>
#include <string>

#include "ITexture.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

/// How a view interprets its texture's layers.
enum class TextureViewDimension {
    Texture2D,      ///< One 2D layer.
    Texture2DArray, ///< Several 2D layers.
    Texture3D,      ///< A 3D texture.
    TextureCube,    ///< Six layers as a cube.
};

/// Which planes of a depth/stencil texture a view sees. Colour textures always use All.
enum class TextureAspect {
    All,         ///< Every plane.
    DepthOnly,   ///< Depth only (e.g. to sample a depth buffer).
    StencilOnly, ///< Stencil only.
};

/// Describes a texture view to create with IDevice::createTextureView.
struct TextureViewDesc {
    TextureRef texture; ///< Texture to view; retained by the view.
    /// Format to interpret texels as; unset uses the texture's format.
    std::optional<Format> format;
    /// How to interpret layers; unset picks the dimension that matches the texture.
    std::optional<TextureViewDimension> dimension;
    TextureAspect aspect = TextureAspect::All; ///< Planes visible through the view.
    TextureSubresourceRange range;             ///< Mip levels and layers visible through the view.
    std::string label;                         ///< Debug name.
};

/// A view of (part of) a texture, used in bind groups and render targets.
///
/// Retains its texture, so the texture stays alive while the view does.
class ITextureView {
public:
    virtual ~ITextureView() = default;

    /// The viewed texture.
    virtual TextureRef getTexture() const = 0;
};

} // namespace rhi
