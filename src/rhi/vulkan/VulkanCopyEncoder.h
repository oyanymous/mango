#pragma once

/**
 * @file
 * @brief rhi::vulkan::VulkanCopyEncoder: records transfer commands into a VulkanCommandBuffer.
 */

#include <memory>

#include "ICopyEncoder.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanCommandBuffer;

/**
 * @brief The Vulkan ICopyEncoder. Created by VulkanCommandBuffer::beginCopy.
 */
class VulkanCopyEncoder final : public ICopyEncoder {
public:
    /**
     * @brief Records into @p command_buffer, which it keeps alive.
     *
     * @param command_buffer The command buffer, in the Encoding state.
     */
    explicit VulkanCopyEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer);
    /**
     * @brief Ends the encoder if end() was not called.
     */
    ~VulkanCopyEncoder() override;

    /**
     * @brief Records vkCmdCopyBuffer and retains both buffers.
     */
    void copyBuffer(BufferCopyDesc desc) override;
    /**
     * @brief Records vkCmdCopyImage (one mip level and layer, all aspects) and retains both textures.
     */
    void copyTexture(TextureCopyDesc desc) override;
    /**
     * @brief Records vkCmdCopyBufferToImage / vkCmdCopyImageToBuffer (one mip level and layer; depth only for
     * depth/stencil formats) and retains the buffer and texture.
     */
    void copyBufferTexture(BufferTextureCopyDesc desc) override;
    void end() override;
private:
    std::shared_ptr<VulkanCommandBuffer> m_command_buffer;
    bool m_ended = false;
    VkCommandBuffer m_native_commands = VK_NULL_HANDLE;
};

} // namespace rhi::vulkan
