/// @file
/// VulkanCopyEncoder.

#include "VulkanCopyEncoder.h"

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"

namespace rhi::vulkan {

VulkanCopyEncoder::VulkanCopyEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer)
    : m_command_buffer(std::move(command_buffer)), m_native_commands(m_command_buffer->handle()) {
}

VulkanCopyEncoder::~VulkanCopyEncoder() {
    end();
}

void VulkanCopyEncoder::copyBuffer(BufferCopyDesc desc) {
    if (m_ended || !desc.src || !desc.dst) {
        return;
    }
    const auto& src = static_cast<const VulkanBuffer&>(*desc.src);
    const auto& dst = static_cast<const VulkanBuffer&>(*desc.dst);

    VkBufferCopy region{};
    region.srcOffset = desc.src_offset;
    region.dstOffset = desc.dst_offset;
    region.size = desc.size == kWholeSize ? src.getDesc().size - desc.src_offset : desc.size;
    vkCmdCopyBuffer(m_native_commands, src.handle(), dst.handle(), 1, &region);

    m_command_buffer->retain(std::move(desc.src));
    m_command_buffer->retain(std::move(desc.dst));
}

void VulkanCopyEncoder::copyTexture(TextureCopyDesc) {
}

void VulkanCopyEncoder::copyBufferTexture(BufferTextureCopyDesc) {
}

void VulkanCopyEncoder::end() {
    if (!m_ended) {
        m_ended = true;
        m_command_buffer->endEncoder();
    }
}

} // namespace rhi::vulkan
