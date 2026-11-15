#pragma once

#include <cstdint>
#include <string>

#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

enum class TextureDimension {
    Texture2D,
    Texture3D,
    TextureCube,
};

enum class TextureUsage : std::uint32_t {
    None = 0,
    CopySrc = 1u << 0,
    CopyDst = 1u << 1,
    Sampled = 1u << 2,
    Storage = 1u << 3,
    RenderTarget = 1u << 4,
    DepthStencil = 1u << 5,
};
MANGO_RHI_FLAGS(TextureUsage)

struct TextureDesc {
    TextureDimension dimension = TextureDimension::Texture2D;
    Format format = Format::Undefined;
    Extent3D extent;
    std::uint32_t mip_level_count = 1;
    std::uint32_t array_layer_count = 1;
    std::uint32_t sample_count = 1;
    TextureUsage usage = TextureUsage::None;
    std::string label;
};

class ITexture {
public:
    virtual ~ITexture() = default;

    virtual TextureDesc getDesc() const = 0;
};

} // namespace rhi
