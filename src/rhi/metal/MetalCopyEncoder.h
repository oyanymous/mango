#pragma once

/**
 * @file
 * @brief rhi::metal::MetalCopyEncoder: copies on an `MTLBlitCommandEncoder`.
 */

#include <memory>

#include "ICopyEncoder.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalCommandBuffer;

/**
 * @brief Records copies into a blit encoder. Ends itself when destroyed.
 */
class MetalCopyEncoder final : public ICopyEncoder {
public:
    /**
     * @brief Wraps an open blit encoder. Created by MetalCommandBuffer::beginCopy.
     *
     * @param command_buffer The command buffer it records into; retains the copied resources.
     * @param encoder The open blit encoder.
     */
    MetalCopyEncoder(std::shared_ptr<MetalCommandBuffer> command_buffer, id<MTLBlitCommandEncoder> encoder);
    ~MetalCopyEncoder() override;

    void copyBuffer(BufferCopyDesc desc) override;

    /**
     * @brief Not implemented yet; reports an error through the debug callback and records nothing.
     */
    void copyTexture(TextureCopyDesc desc) override;

    /**
     * @brief Not implemented yet; reports an error through the debug callback and records nothing.
     */
    void copyBufferTexture(BufferTextureCopyDesc desc) override;

    void end() override;
private:
    std::shared_ptr<MetalCommandBuffer> m_command_buffer;
    id<MTLBlitCommandEncoder> m_encoder = nil;
    bool m_ended = false;
};

} // namespace rhi::metal
