#include "VulkanComputeEncoder.h"

#include "VulkanBindGroup.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"

namespace rhi::vulkan {

VulkanComputeEncoder::VulkanComputeEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer)
    : m_command_buffer(std::move(command_buffer)), m_commands(m_command_buffer->commands()) {
}

VulkanComputeEncoder::~VulkanComputeEncoder() {
    end();
}

void VulkanComputeEncoder::setPipeline(PipelineRef pipeline) {
    m_commands.bindPipeline(vk::PipelineBindPoint::eCompute, static_cast<const VulkanPipeline&>(*pipeline).handle());
    m_pipeline = pipeline;
    m_command_buffer->retain(std::move(pipeline));
}

void VulkanComputeEncoder::setBindGroup(BindGroupBinding binding) {
    if (!m_pipeline) {
        return;
    }
    m_commands.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                                  static_cast<const VulkanPipeline&>(*m_pipeline).layoutHandle(), binding.index,
                                  static_cast<const VulkanBindGroup&>(*binding.bind_group).handle(), nullptr);
    m_command_buffer->retain(std::move(binding.bind_group));
}

void VulkanComputeEncoder::dispatch(DispatchDesc desc) {
    m_commands.dispatch(desc.x, desc.y, desc.z);
}

void VulkanComputeEncoder::end() {
    if (!m_ended) {
        m_ended = true;
        m_command_buffer->endEncoder();
    }
}

} // namespace rhi::vulkan
