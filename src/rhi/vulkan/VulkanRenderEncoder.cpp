/**
 * @file
 * @brief VulkanRenderEncoder.
 */

#include "VulkanRenderEncoder.h"

#include "VulkanCommandBuffer.h"

namespace rhi::vulkan {

VulkanRenderEncoder::VulkanRenderEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer)
    : m_command_buffer(std::move(command_buffer)), m_native_commands(m_command_buffer->handle()) {
}

VulkanRenderEncoder::~VulkanRenderEncoder() {
    end();
}

// shortcut: the recording calls below are no-ops until pipelines (step 10) and bind groups (step 11) exist.
void VulkanRenderEncoder::setPipeline(PipelineRef) {
}

void VulkanRenderEncoder::setBindGroup(BindGroupBinding) {
}

void VulkanRenderEncoder::setVertexBuffers(VertexBufferBindings) {
}

void VulkanRenderEncoder::setIndexBuffer(IndexBufferBinding) {
}

void VulkanRenderEncoder::draw(DrawDesc) {
}

void VulkanRenderEncoder::drawIndexed(DrawIndexedDesc) {
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
