/**
 * @file
 * @brief VulkanRenderEncoder.
 */

#include "VulkanRenderEncoder.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "VulkanBindGroup.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"

namespace rhi::vulkan {

VulkanRenderEncoder::VulkanRenderEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer, Extent2D extent)
    : m_command_buffer(std::move(command_buffer)), m_native_commands(m_command_buffer->handle()), m_extent(extent) {
}

VulkanRenderEncoder::~VulkanRenderEncoder() {
    end();
}

void VulkanRenderEncoder::setPipeline(PipelineRef pipeline) {
    vkCmdBindPipeline(m_native_commands, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      static_cast<const VulkanPipeline&>(*pipeline).handle());
    m_pipeline = pipeline;
    m_command_buffer->retain(std::move(pipeline));
}

void VulkanRenderEncoder::setBindGroup(BindGroupBinding binding) {
    if (!m_pipeline) {
        return; // Vulkan binds sets through a pipeline layout; see the header.
    }
    const VkDescriptorSet set = static_cast<const VulkanBindGroup&>(*binding.bind_group).handle();
    vkCmdBindDescriptorSets(m_native_commands, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            static_cast<const VulkanPipeline&>(*m_pipeline).layoutHandle(), binding.index, 1, &set, 0,
                            nullptr);
    m_command_buffer->retain(std::move(binding.bind_group));
}

void VulkanRenderEncoder::setVertexBuffers(VertexBufferBindings bindings) {
    std::vector<VkBuffer> buffers;
    std::vector<VkDeviceSize> offsets;
    for (VertexBufferBinding& binding : bindings.buffers) {
        buffers.push_back(static_cast<const VulkanBuffer&>(*binding.buffer).handle());
        offsets.push_back(binding.offset);
        m_command_buffer->retain(std::move(binding.buffer));
    }
    vkCmdBindVertexBuffers(m_native_commands, bindings.first_slot, static_cast<std::uint32_t>(buffers.size()),
                           buffers.data(), offsets.data());
}

void VulkanRenderEncoder::setIndexBuffer(IndexBufferBinding binding) {
    vkCmdBindIndexBuffer(m_native_commands, static_cast<const VulkanBuffer&>(*binding.buffer).handle(), binding.offset,
                         binding.format == IndexFormat::Uint16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    m_command_buffer->retain(std::move(binding.buffer));
}

void VulkanRenderEncoder::draw(DrawDesc desc) {
    if (m_scissor_empty) {
        return;
    }
    vkCmdDraw(m_native_commands, desc.vertex_count, desc.instance_count, desc.first_vertex, desc.first_instance);
}

void VulkanRenderEncoder::drawIndexed(DrawIndexedDesc desc) {
    if (m_scissor_empty) {
        return;
    }
    vkCmdDrawIndexed(m_native_commands, desc.index_count, desc.instance_count, desc.first_index, desc.vertex_offset,
                     desc.first_instance);
}

void VulkanRenderEncoder::setViewport(Viewport viewport) {
    const VkViewport native{viewport.x,      viewport.y,         viewport.width,
                            viewport.height, viewport.min_depth, viewport.max_depth};
    vkCmdSetViewport(m_native_commands, 0, 1, &native);
}

void VulkanRenderEncoder::setScissor(ScissorRect rect) {
    // Vulkan requires the scissor inside the framebuffer, so clamp; an empty result skips draws instead.
    const std::int64_t width = m_extent.width;
    const std::int64_t height = m_extent.height;
    const std::int64_t x0 = std::clamp<std::int64_t>(rect.x, 0, width);
    const std::int64_t y0 = std::clamp<std::int64_t>(rect.y, 0, height);
    const std::int64_t x1 = std::clamp<std::int64_t>(static_cast<std::int64_t>(rect.x) + rect.width, 0, width);
    const std::int64_t y1 = std::clamp<std::int64_t>(static_cast<std::int64_t>(rect.y) + rect.height, 0, height);
    m_scissor_empty = x1 <= x0 || y1 <= y0;
    if (m_scissor_empty) {
        return;
    }
    const VkRect2D native{{static_cast<std::int32_t>(x0), static_cast<std::int32_t>(y0)},
                          {static_cast<std::uint32_t>(x1 - x0), static_cast<std::uint32_t>(y1 - y0)}};
    vkCmdSetScissor(m_native_commands, 0, 1, &native);
}

void VulkanRenderEncoder::setStencilReference(std::uint32_t reference) {
    vkCmdSetStencilReference(m_native_commands, VK_STENCIL_FACE_FRONT_AND_BACK, reference & 0xffu);
}

void VulkanRenderEncoder::end() {
    if (m_ended) {
        return;
    }
    m_ended = true;
    vkCmdEndRenderPass(m_native_commands);
    m_command_buffer->endEncoder();
}

} // namespace rhi::vulkan
