#pragma once

#include <cstdint>
#include <string>

#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

enum class TextureDimension {
    Texture2D,   /**< 2D, optionally an array (TextureDesc::array_layer_count > 1). */
    Texture3D,   /**< 3D (volume). */
    TextureCube, /**< Cube map: 6 layers per cube. */
};

/// How a texture may be used, as a bit set. Using a texture in a way not listed here is invalid.
enum class TextureUsage : std::uint32_t {
    None = 0,
    CopySrc = 1u << 0,      /**< Source of copies. */
    CopyDst = 1u << 1,      /**< Destination of copies (e.g. uploads). */
    Sampled = 1u << 2,      /**< Read in shaders through a sampler. */
    Storage = 1u << 3,      /**< Read-write storage image in shaders. */
    RenderTarget = 1u << 4, /**< Colour attachment of a render target. */
    DepthStencil = 1u << 5, /**< Depth/stencil attachment of a render target. */
};
MANGO_RHI_FLAGS(TextureUsage)

/// Describes a texture to create with IDevice::createTexture.
struct TextureDesc {
    TextureDimension dimension = TextureDimension::Texture2D;
    Format format = Format::Undefined;       /**< Texel format; must not be Undefined. */
    Extent3D extent;                         /**< Size of mip level 0. */
    std::uint32_t mip_level_count = 1;       /**< Number of mip levels. */
    std::uint32_t array_layer_count = 1;     /**< Array layers; 6 per cube for TextureCube. */
    std::uint32_t sample_count = 1;          /**< MSAA samples (1, 2, 4, 8). */
    TextureUsage usage = TextureUsage::None; /**< Every way the texture will be used. */
    std::string label;                       /**< Debug name shown in tools and validation messages. */
};

/**
 * An image on the GPU. Shaders and render targets access it through an ITextureView.
 *
 * Created by IDevice::createTexture, or owned by a swapchain (its backbuffers). A texture starts in the
 * ResourceState::Undefined state; use ICommandBuffer::barrier to move it into the state the next use needs.
 */
class ITexture {
public:
    virtual ~ITexture() = default;

    virtual TextureDesc getDesc() const = 0;
};

} // namespace rhi
