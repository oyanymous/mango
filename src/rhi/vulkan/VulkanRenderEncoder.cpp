/**
 * @file
 * @brief VulkanRenderEncoder.
 */

#include "VulkanRenderEncoder.h"

#include <vector>

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"

namespace rhi::vulkan {

VulkanRenderEncoder::VulkanRenderEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer)
    : m_command_buffer(std::move(command_buffer)), m_native_commands(m_command_buffer->handle()) {
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

// shortcut: a no-op until bind groups exist (step 11).
void VulkanRenderEncoder::setBindGroup(BindGroupBinding) {
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
    vkCmdDraw(m_native_commands, desc.vertex_count, desc.instance_count, desc.first_vertex, desc.first_instance);
}

void VulkanRenderEncoder::drawIndexed(DrawIndexedDesc desc) {
    vkCmdDrawIndexed(m_native_commands, desc.index_count, desc.instance_count, desc.first_index, desc.vertex_offset,
                     desc.first_instance);
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
