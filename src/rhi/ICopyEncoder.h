#pragma once

#include <cstdint>
#include <vector>

#include "IBuffer.h"
#include "ITexture.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

struct BufferCopyDesc {
    BufferRef src;
    std::uint64_t src_offset = 0;
    BufferRef dst;
    std::uint64_t dst_offset = 0;
    std::uint64_t size = kWholeSize;
};

// One mip level / array layer of a texture, starting at origin.
struct TextureLocation {
    TextureRef texture;
    std::uint32_t mip_level = 0;
    std::uint32_t array_layer = 0;
    Offset3D origin;
};

struct TextureCopyDesc {
    TextureLocation src;
    TextureLocation dst;
    Extent3D extent;
};

enum class CopyDirection {
    BufferToTexture,
    TextureToBuffer,
};

// How texel rows are laid out in the buffer side of a buffer <-> texture copy.
struct BufferTextureLayout {
    BufferRef buffer;
    std::uint64_t offset = 0;
    std::uint32_t row_length = 0;   // in texels; 0 = tightly packed (extent.width)
    std::uint32_t image_height = 0; // in rows; 0 = tightly packed (extent.height)
};

struct BufferTextureCopyDesc {
    CopyDirection direction = CopyDirection::BufferToTexture;
    BufferTextureLayout buffer;
    TextureLocation texture;
    Extent3D extent;
};

// Records transfers. end() must be called before the command buffer records anything else.
class ICopyEncoder {
public:
    virtual ~ICopyEncoder() = default;

    virtual void copyBuffer(BufferCopyDesc desc) = 0;
    virtual void copyTexture(TextureCopyDesc desc) = 0;
    virtual void copyBufferTexture(BufferTextureCopyDesc desc) = 0;
    virtual void end() = 0;
};

} // namespace rhi
