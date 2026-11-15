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
    : m_command_buffer(std::move(command_buffer)), m_extent(extent), m_commands(m_command_buffer->commands()) {
}

VulkanRenderEncoder::~VulkanRenderEncoder() {
    end();
}

void VulkanRenderEncoder::setPipeline(PipelineRef pipeline) {
    m_commands.bindPipeline(vk::PipelineBindPoint::eGraphics, static_cast<const VulkanPipeline&>(*pipeline).handle());
    m_pipeline = pipeline;
    m_command_buffer->retain(std::move(pipeline));
}

void VulkanRenderEncoder::setBindGroup(BindGroupBinding binding) {
    if (!m_pipeline) {
        return; // Vulkan binds sets through a pipeline layout; see the header.
    }
    m_commands.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                  static_cast<const VulkanPipeline&>(*m_pipeline).layoutHandle(), binding.index,
                                  static_cast<const VulkanBindGroup&>(*binding.bind_group).handle(), nullptr);
    m_command_buffer->retain(std::move(binding.bind_group));
}

void VulkanRenderEncoder::setVertexBuffers(VertexBufferBindings bindings) {
    std::vector<vk::Buffer> buffers;
    std::vector<vk::DeviceSize> offsets;
    for (VertexBufferBinding& binding : bindings.buffers) {
        buffers.push_back(static_cast<const VulkanBuffer&>(*binding.buffer).handle());
        offsets.push_back(binding.offset);
        m_command_buffer->retain(std::move(binding.buffer));
    }
    m_commands.bindVertexBuffers(bindings.first_slot, buffers, offsets);
}

void VulkanRenderEncoder::setIndexBuffer(IndexBufferBinding binding) {
    m_commands.bindIndexBuffer(static_cast<const VulkanBuffer&>(*binding.buffer).handle(), binding.offset,
                               binding.format == IndexFormat::Uint16 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
    m_command_buffer->retain(std::move(binding.buffer));
}

void VulkanRenderEncoder::draw(DrawDesc desc) {
    if (m_scissor_empty) {
        return;
    }
    m_commands.draw(desc.vertex_count, desc.instance_count, desc.first_vertex, desc.first_instance);
}

void VulkanRenderEncoder::drawIndexed(DrawIndexedDesc desc) {
    if (m_scissor_empty) {
        return;
    }
    m_commands.drawIndexed(desc.index_count, desc.instance_count, desc.first_index, desc.vertex_offset,
                           desc.first_instance);
}

void VulkanRenderEncoder::setViewport(Viewport viewport) {
    m_commands.setViewport(0, vk::Viewport{
                                  .x = viewport.x,
                                  .y = viewport.y,
                                  .width = viewport.width,
                                  .height = viewport.height,
                                  .minDepth = viewport.min_depth,
                                  .maxDepth = viewport.max_depth,
                              });
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
    m_commands.setScissor(0, vk::Rect2D{
                                 .offset = {static_cast<std::int32_t>(x0), static_cast<std::int32_t>(y0)},
                                 .extent = {static_cast<std::uint32_t>(x1 - x0), static_cast<std::uint32_t>(y1 - y0)},
                             });
}

void VulkanRenderEncoder::setStencilReference(std::uint32_t reference) {
    m_commands.setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, reference & 0xffu);
}

void VulkanRenderEncoder::end() {
    if (m_ended) {
        return;
    }
    m_ended = true;
    m_commands.endRenderPass();
    m_command_buffer->endEncoder();
}

} // namespace rhi::vulkan
