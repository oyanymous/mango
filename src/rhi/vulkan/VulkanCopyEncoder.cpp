/**
 * @file
 * @brief VulkanCopyEncoder.
 */

#include "VulkanCopyEncoder.h"

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanTexture.h"

namespace rhi::vulkan {

namespace {

/**
 * @brief The one mip level and array layer a TextureLocation names.
 */
VkImageSubresourceLayers toSubresourceLayers(const TextureLocation& location, VkImageAspectFlags aspect) {
    return {aspect, location.mip_level, location.array_layer, 1};
}

} // namespace

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

void VulkanCopyEncoder::copyTexture(TextureCopyDesc desc) {
    if (m_ended || !desc.src.texture || !desc.dst.texture) {
        return;
    }
    const auto& src = static_cast<const VulkanTexture&>(*desc.src.texture);
    const auto& dst = static_cast<const VulkanTexture&>(*desc.dst.texture);

    VkImageCopy region{};
    region.srcSubresource = toSubresourceLayers(desc.src, aspectsOf(src.getDesc().format));
    region.srcOffset = {desc.src.origin.x, desc.src.origin.y, desc.src.origin.z};
    region.dstSubresource = toSubresourceLayers(desc.dst, aspectsOf(dst.getDesc().format));
    region.dstOffset = {desc.dst.origin.x, desc.dst.origin.y, desc.dst.origin.z};
    region.extent = {desc.extent.width, desc.extent.height, desc.extent.depth};
    vkCmdCopyImage(m_native_commands, src.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.handle(),
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    m_command_buffer->retain(std::move(desc.src.texture));
    m_command_buffer->retain(std::move(desc.dst.texture));
}

void VulkanCopyEncoder::copyBufferTexture(BufferTextureCopyDesc desc) {
    if (m_ended || !desc.buffer.buffer || !desc.texture.texture) {
        return;
    }
    const auto& buffer = static_cast<const VulkanBuffer&>(*desc.buffer.buffer);
    const auto& texture = static_cast<const VulkanTexture&>(*desc.texture.texture);

    // Buffer <-> image copies take exactly one aspect; for depth/stencil formats that is depth.
    VkImageAspectFlags aspect = aspectsOf(texture.getDesc().format);
    if (aspect & VK_IMAGE_ASPECT_DEPTH_BIT) {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    VkBufferImageCopy region{};
    region.bufferOffset = desc.buffer.offset;
    region.bufferRowLength = desc.buffer.row_length; // 0 means tightly packed in both APIs
    region.bufferImageHeight = desc.buffer.image_height;
    region.imageSubresource = toSubresourceLayers(desc.texture, aspect);
    region.imageOffset = {desc.texture.origin.x, desc.texture.origin.y, desc.texture.origin.z};
    region.imageExtent = {desc.extent.width, desc.extent.height, desc.extent.depth};
    if (desc.direction == CopyDirection::BufferToTexture) {
        vkCmdCopyBufferToImage(m_native_commands, buffer.handle(), texture.handle(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    } else {
        vkCmdCopyImageToBuffer(m_native_commands, texture.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               buffer.handle(), 1, &region);
    }

    m_command_buffer->retain(std::move(desc.buffer.buffer));
    m_command_buffer->retain(std::move(desc.texture.texture));
}

void VulkanCopyEncoder::end() {
    if (!m_ended) {
        m_ended = true;
        m_command_buffer->endEncoder();
    }
}

} // namespace rhi::vulkan
