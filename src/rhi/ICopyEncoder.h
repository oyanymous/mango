#pragma once

/**
 * @file
 * @brief rhi::ICopyEncoder: records copies between buffers and textures.
 */

#include <cstdint>
#include <vector>

#include "IBuffer.h"
#include "ITexture.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

/**
 * @brief A buffer-to-buffer copy.
 */
struct BufferCopyDesc {
    BufferRef src;                   /**< Source, with BufferUsage::CopySrc. */
    std::uint64_t src_offset = 0;    /**< First source byte. */
    BufferRef dst;                   /**< Destination, with BufferUsage::CopyDst. */
    std::uint64_t dst_offset = 0;    /**< First destination byte. */
    std::uint64_t size = kWholeSize; /**< Bytes to copy; kWholeSize for the rest of the source. */
};

/**
 * @brief One mip level / array layer of a texture, starting at `origin`.
 */
struct TextureLocation {
    TextureRef texture;            /**< The texture. */
    std::uint32_t mip_level = 0;   /**< Mip level. */
    std::uint32_t array_layer = 0; /**< Array layer. */
    Offset3D origin;               /**< Texel offset of the region. */
};

/**
 * @brief A texture-to-texture copy.
 */
struct TextureCopyDesc {
    TextureLocation src; /**< Source region start; texture in ResourceState::CopySrc. */
    TextureLocation dst; /**< Destination region start; texture in ResourceState::CopyDst. */
    Extent3D extent;     /**< Size of the region in texels. */
};

/**
 * @brief The direction of a buffer <-> texture copy.
 */
enum class CopyDirection {
    BufferToTexture, /**< Upload: buffer -> texture. */
    TextureToBuffer, /**< Readback: texture -> buffer. */
};

/**
 * @brief How texel rows are laid out on the buffer side of a buffer <-> texture copy.
 */
struct BufferTextureLayout {
    BufferRef buffer;               /**< The buffer. */
    std::uint64_t offset = 0;       /**< Byte offset of the first texel. */
    std::uint32_t row_length = 0;   /**< Row pitch in texels; 0 means tightly packed (extent.width). */
    std::uint32_t image_height = 0; /**< Rows per 2D slice; 0 means tightly packed (extent.height). */
};

/**
 * @brief A copy between a buffer and a texture region.
 */
struct BufferTextureCopyDesc {
    CopyDirection direction = CopyDirection::BufferToTexture; /**< Which way to copy. */
    BufferTextureLayout buffer;                               /**< The buffer side. */
    TextureLocation texture;                                  /**< The texture side (region start). */
    Extent3D extent;                                          /**< Size of the region in texels. */
};

/**
 * @brief Records transfers. Returned by ICommandBuffer::beginCopy.
 *
 * @details
 * Textures must be in ResourceState::CopySrc / CopyDst as appropriate. end() must be called before the
 * command buffer records anything else.
 */
class ICopyEncoder {
public:
    virtual ~ICopyEncoder() = default;

    /**
     * @brief Copies bytes between buffers.
     *
     * @param desc Source, destination and range.
     */
    virtual void copyBuffer(BufferCopyDesc desc) = 0;
    /**
     * @brief Copies a texel region between textures of compatible formats.
     *
     * @param desc Source and destination regions.
     */
    virtual void copyTexture(TextureCopyDesc desc) = 0;
    /**
     * @brief Copies a texel region between a buffer and a texture, in either direction.
     *
     * @param desc Direction, buffer layout and texture region.
     */
    virtual void copyBufferTexture(BufferTextureCopyDesc desc) = 0;
    /**
     * @brief Ends copy recording. The encoder must not be used afterwards.
     */
    virtual void end() = 0;
};

} // namespace rhi
