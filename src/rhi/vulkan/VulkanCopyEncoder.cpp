#include "VulkanCopyEncoder.h"

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanTexture.h"

namespace rhi::vulkan {

namespace {

vk::ImageSubresourceLayers toSubresourceLayers(const TextureLocation& location, vk::ImageAspectFlags aspect) {
    return {
        .aspectMask = aspect, .mipLevel = location.mip_level, .baseArrayLayer = location.array_layer, .layerCount = 1};
}

vk::Offset3D toOffset(const TextureLocation& location) {
    return {location.origin.x, location.origin.y, location.origin.z};
}

} // namespace

VulkanCopyEncoder::VulkanCopyEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer)
    : m_command_buffer(std::move(command_buffer)), m_commands(m_command_buffer->commands()) {
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

    const vk::BufferCopy region{
        .srcOffset = desc.src_offset,
        .dstOffset = desc.dst_offset,
        .size = desc.size == kWholeSize ? src.getDesc().size - desc.src_offset : desc.size,
    };
    m_commands.copyBuffer(src.handle(), dst.handle(), region);

    m_command_buffer->retain(std::move(desc.src));
    m_command_buffer->retain(std::move(desc.dst));
}

void VulkanCopyEncoder::copyTexture(TextureCopyDesc desc) {
    if (m_ended || !desc.src.texture || !desc.dst.texture) {
        return;
    }
    const auto& src = static_cast<const VulkanTexture&>(*desc.src.texture);
    const auto& dst = static_cast<const VulkanTexture&>(*desc.dst.texture);

    const vk::ImageCopy region{
        .srcSubresource = toSubresourceLayers(desc.src, aspectsOf(src.getDesc().format)),
        .srcOffset = toOffset(desc.src),
        .dstSubresource = toSubresourceLayers(desc.dst, aspectsOf(dst.getDesc().format)),
        .dstOffset = toOffset(desc.dst),
        .extent = {desc.extent.width, desc.extent.height, desc.extent.depth},
    };
    m_commands.copyImage(src.handle(), vk::ImageLayout::eTransferSrcOptimal, dst.handle(),
                         vk::ImageLayout::eTransferDstOptimal, region);

    m_command_buffer->retain(std::move(desc.src.texture));
    m_command_buffer->retain(std::move(desc.dst.texture));
}

void VulkanCopyEncoder::copyBufferTexture(BufferTextureCopyDesc desc) {
    if (m_ended || !desc.buffer.buffer || !desc.texture.texture) {
        return;
    }
    const auto& buffer = static_cast<const VulkanBuffer&>(*desc.buffer.buffer);
    const auto& texture = static_cast<const VulkanTexture&>(*desc.texture.texture);

    vk::ImageAspectFlags aspect = aspectsOf(texture.getDesc().format);
    if (aspect & vk::ImageAspectFlagBits::eDepth) {
        aspect = vk::ImageAspectFlagBits::eDepth;
    }

    const vk::BufferImageCopy region{
        .bufferOffset = desc.buffer.offset,
        .bufferRowLength = desc.buffer.row_length,
        .bufferImageHeight = desc.buffer.image_height,
        .imageSubresource = toSubresourceLayers(desc.texture, aspect),
        .imageOffset = toOffset(desc.texture),
        .imageExtent = {desc.extent.width, desc.extent.height, desc.extent.depth},
    };
    if (desc.direction == CopyDirection::BufferToTexture) {
        m_commands.copyBufferToImage(buffer.handle(), texture.handle(), vk::ImageLayout::eTransferDstOptimal, region);
    } else {
        m_commands.copyImageToBuffer(texture.handle(), vk::ImageLayout::eTransferSrcOptimal, buffer.handle(), region);
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
