/**
 * @file
 * @brief MetalCopyEncoder: buffer copies on a blit encoder.
 */

#include "MetalCopyEncoder.h"

#include <utility>

#include "MetalBuffer.h"
#include "MetalCommandBuffer.h"
#include "MetalTexture.h"

namespace rhi::metal {

namespace {

/**
 * @brief A texel offset as an `MTLOrigin`.
 */
MTLOrigin toMTLOrigin(const Offset3D& offset) {
    return MTLOriginMake(static_cast<NSUInteger>(offset.x), static_cast<NSUInteger>(offset.y),
                         static_cast<NSUInteger>(offset.z));
}

/**
 * @brief An extent as an `MTLSize`.
 */
MTLSize toMTLSize(const Extent3D& extent) {
    return MTLSizeMake(extent.width, extent.height, extent.depth);
}

} // namespace

MetalCopyEncoder::MetalCopyEncoder(std::shared_ptr<MetalCommandBuffer> command_buffer,
                                   id<MTLBlitCommandEncoder> encoder)
    : m_command_buffer(std::move(command_buffer)), m_encoder(encoder) {
}

MetalCopyEncoder::~MetalCopyEncoder() {
    end();
}

void MetalCopyEncoder::copyBuffer(BufferCopyDesc desc) {
    if (m_ended || !desc.src || !desc.dst) {
        return;
    }
    const auto& src = static_cast<const MetalBuffer&>(*desc.src);
    const auto& dst = static_cast<const MetalBuffer&>(*desc.dst);
    const std::uint64_t size = desc.size == kWholeSize ? src.getDesc().size - desc.src_offset : desc.size;
    [m_encoder copyFromBuffer:src.handle()
                 sourceOffset:desc.src_offset
                     toBuffer:dst.handle()
            destinationOffset:desc.dst_offset
                         size:size];

    m_command_buffer->retain(std::move(desc.src));
    m_command_buffer->retain(std::move(desc.dst));
}

void MetalCopyEncoder::copyTexture(TextureCopyDesc desc) {
    if (m_ended || !desc.src.texture || !desc.dst.texture) {
        return;
    }
    const auto& src = static_cast<const MetalTexture&>(*desc.src.texture);
    const auto& dst = static_cast<const MetalTexture&>(*desc.dst.texture);
    [m_encoder copyFromTexture:src.handle()
                   sourceSlice:desc.src.array_layer
                   sourceLevel:desc.src.mip_level
                  sourceOrigin:toMTLOrigin(desc.src.origin)
                    sourceSize:toMTLSize(desc.extent)
                     toTexture:dst.handle()
              destinationSlice:desc.dst.array_layer
              destinationLevel:desc.dst.mip_level
             destinationOrigin:toMTLOrigin(desc.dst.origin)];

    m_command_buffer->retain(std::move(desc.src.texture));
    m_command_buffer->retain(std::move(desc.dst.texture));
}

void MetalCopyEncoder::copyBufferTexture(BufferTextureCopyDesc desc) {
    if (m_ended || !desc.buffer.buffer || !desc.texture.texture) {
        return;
    }
    const auto& buffer = static_cast<const MetalBuffer&>(*desc.buffer.buffer);
    const auto& texture = static_cast<const MetalTexture&>(*desc.texture.texture);
    const TextureDesc texture_desc = texture.getDesc();

    // 0 means tightly packed in the RHI, as in Vulkan; Metal wants the pitches in bytes.
    const std::uint32_t row_texels = desc.buffer.row_length != 0 ? desc.buffer.row_length : desc.extent.width;
    const std::uint32_t image_rows = desc.buffer.image_height != 0 ? desc.buffer.image_height : desc.extent.height;
    const NSUInteger bytes_per_row = static_cast<NSUInteger>(row_texels) * bytesPerTexel(texture_desc.format);
    const NSUInteger bytes_per_image = bytes_per_row * image_rows;
    // Buffer copies take one plane; for depth/stencil formats that is depth, as in the Vulkan backend.
    const MTLBlitOption options =
        hasStencil(texture_desc.format) ? MTLBlitOptionDepthFromDepthStencil : MTLBlitOptionNone;

    if (desc.direction == CopyDirection::BufferToTexture) {
        [m_encoder copyFromBuffer:buffer.handle()
                     sourceOffset:desc.buffer.offset
                sourceBytesPerRow:bytes_per_row
              sourceBytesPerImage:bytes_per_image
                       sourceSize:toMTLSize(desc.extent)
                        toTexture:texture.handle()
                 destinationSlice:desc.texture.array_layer
                 destinationLevel:desc.texture.mip_level
                destinationOrigin:toMTLOrigin(desc.texture.origin)
                          options:options];
    } else {
        [m_encoder copyFromTexture:texture.handle()
                         sourceSlice:desc.texture.array_layer
                         sourceLevel:desc.texture.mip_level
                        sourceOrigin:toMTLOrigin(desc.texture.origin)
                          sourceSize:toMTLSize(desc.extent)
                            toBuffer:buffer.handle()
                   destinationOffset:desc.buffer.offset
              destinationBytesPerRow:bytes_per_row
            destinationBytesPerImage:bytes_per_image
                             options:options];
    }

    m_command_buffer->retain(std::move(desc.buffer.buffer));
    m_command_buffer->retain(std::move(desc.texture.texture));
}

void MetalCopyEncoder::end() {
    if (!m_ended) {
        m_ended = true;
        [m_encoder endEncoding];
        m_command_buffer->endEncoder();
    }
}

} // namespace rhi::metal
