#pragma once

#include <memory>

#include "ICopyEncoder.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanCommandBuffer;

class VulkanCopyEncoder final : public ICopyEncoder {
public:
    explicit VulkanCopyEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer);

    ~VulkanCopyEncoder() override;

    void copyBuffer(BufferCopyDesc desc) override;

    void copyTexture(TextureCopyDesc desc) override;

    void copyBufferTexture(BufferTextureCopyDesc desc) override;
    void end() override;
private:
    std::shared_ptr<VulkanCommandBuffer> m_command_buffer;
    bool m_ended = false;
    const vk::raii::CommandBuffer& m_commands;
};

} // namespace rhi::vulkan
