/**
 * @file
 * @brief MetalCopyEncoder: buffer copies on a blit encoder.
 */

#include "MetalCopyEncoder.h"

#include <utility>

#include "MetalBuffer.h"
#include "MetalCommandBuffer.h"

namespace rhi::metal {

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

void MetalCopyEncoder::copyTexture(TextureCopyDesc) {
    m_command_buffer->reportError("MetalCopyEncoder::copyTexture is not implemented yet");
}

void MetalCopyEncoder::copyBufferTexture(BufferTextureCopyDesc) {
    m_command_buffer->reportError("MetalCopyEncoder::copyBufferTexture is not implemented yet");
}

void MetalCopyEncoder::end() {
    if (!m_ended) {
        m_ended = true;
        [m_encoder endEncoding];
        m_command_buffer->endEncoder();
    }
}

} // namespace rhi::metal
