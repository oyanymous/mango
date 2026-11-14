#pragma once

/// @file
/// rhi::vulkan::VulkanCopyEncoder: records transfer commands into a VulkanCommandBuffer.

#include <memory>

#include "ICopyEncoder.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanCommandBuffer;

/// The Vulkan ICopyEncoder. Created by VulkanCommandBuffer::beginCopy.
class VulkanCopyEncoder final : public ICopyEncoder {
public:
    /// Records into @p command_buffer, which it keeps alive.
    /// @param command_buffer The command buffer, in the Encoding state.
    explicit VulkanCopyEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer);
    /// Ends the encoder if end() was not called.
    ~VulkanCopyEncoder() override;

    /// Records vkCmdCopyBuffer and retains both buffers.
    void copyBuffer(BufferCopyDesc desc) override;
    /// Not implemented yet (step 7); records nothing.
    void copyTexture(TextureCopyDesc desc) override;
    /// Not implemented yet (step 7); records nothing.
    void copyBufferTexture(BufferTextureCopyDesc desc) override;
    void end() override;
private:
    std::shared_ptr<VulkanCommandBuffer> m_command_buffer;
    bool m_ended = false;
    VkCommandBuffer m_native_commands = VK_NULL_HANDLE;
};

} // namespace rhi::vulkan
