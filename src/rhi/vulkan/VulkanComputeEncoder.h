#pragma once

#include <memory>

#include "IComputeEncoder.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanCommandBuffer;

class VulkanComputeEncoder final : public IComputeEncoder {
public:
    explicit VulkanComputeEncoder(std::shared_ptr<VulkanCommandBuffer> command_buffer);

    ~VulkanComputeEncoder() override;

    void setPipeline(PipelineRef pipeline) override;
    void setBindGroup(BindGroupBinding binding) override;
    void dispatch(DispatchDesc desc) override;

    void end() override;
private:
    std::shared_ptr<VulkanCommandBuffer> m_command_buffer;
    PipelineRef m_pipeline;
    bool m_ended = false;
    const vk::raii::CommandBuffer& m_commands;
};

} // namespace rhi::vulkan
