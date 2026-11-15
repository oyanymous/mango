#pragma once

#include <memory>

#include "ICopyEncoder.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalCommandBuffer;

class MetalCopyEncoder final : public ICopyEncoder {
public:
    MetalCopyEncoder(std::shared_ptr<MetalCommandBuffer> command_buffer, id<MTLBlitCommandEncoder> encoder);
    ~MetalCopyEncoder() override;

    void copyBuffer(BufferCopyDesc desc) override;
    void copyTexture(TextureCopyDesc desc) override;

    void copyBufferTexture(BufferTextureCopyDesc desc) override;

    void end() override;
private:
    std::shared_ptr<MetalCommandBuffer> m_command_buffer;
    id<MTLBlitCommandEncoder> m_encoder = nil;
    bool m_ended = false;
};

} // namespace rhi::metal
