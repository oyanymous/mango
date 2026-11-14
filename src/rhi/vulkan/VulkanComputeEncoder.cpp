/**
 * @file
 * @brief VulkanComputeEncoder.
 */

#include "VulkanComputeEncoder.h"

#include "VulkanBindGroup.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"

namespace rhi::vulkan {

VulkanComputeEncoder::VulkanComputeEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer)
    : m_command_buffer(std::move(command_buffer)), m_native_commands(m_command_buffer->handle()) {
}

VulkanComputeEncoder::~VulkanComputeEncoder() {
    end();
}

void VulkanComputeEncoder::setPipeline(PipelineRef pipeline) {
    vkCmdBindPipeline(m_native_commands, VK_PIPELINE_BIND_POINT_COMPUTE,
                      static_cast<const VulkanPipeline&>(*pipeline).handle());
    m_pipeline = pipeline;
    m_command_buffer->retain(std::move(pipeline));
}

void VulkanComputeEncoder::setBindGroup(BindGroupBinding binding) {
    if (!m_pipeline) {
        return; // Vulkan binds sets through a pipeline layout; see the header.
    }
    const VkDescriptorSet set = static_cast<const VulkanBindGroup&>(*binding.bind_group).handle();
    vkCmdBindDescriptorSets(m_native_commands, VK_PIPELINE_BIND_POINT_COMPUTE,
                            static_cast<const VulkanPipeline&>(*m_pipeline).layoutHandle(), binding.index, 1, &set, 0,
                            nullptr);
    m_command_buffer->retain(std::move(binding.bind_group));
}

void VulkanComputeEncoder::dispatch(DispatchDesc desc) {
    vkCmdDispatch(m_native_commands, desc.x, desc.y, desc.z);
}

void VulkanComputeEncoder::end() {
    if (!m_ended) {
        m_ended = true;
        m_command_buffer->endEncoder();
    }
}

} // namespace rhi::vulkan
