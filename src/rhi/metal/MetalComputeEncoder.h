#pragma once

#include <memory>

#include "IComputeEncoder.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalCommandBuffer;
class MetalPipeline;

/**
 * Records compute dispatches. Ends itself when destroyed.
 *
 * dispatch() runs `x * y * z` threadgroups of the pipeline's workgroup size (the shader's `local_size`), like
 * `vkCmdDispatch`. Bind groups bind through the current pipeline's layout, so set the pipeline first.
 */
class MetalComputeEncoder final : public IComputeEncoder {
public:
    /**
     * Wraps an open compute encoder. Created by MetalCommandBuffer::beginCompute.
     *
     * @param command_buffer The command buffer it records into; retains the resources used.
     */
    MetalComputeEncoder(std::shared_ptr<MetalCommandBuffer> command_buffer, id<MTLComputeCommandEncoder> encoder);
    ~MetalComputeEncoder() override;

    void setPipeline(PipelineRef pipeline) override;
    void setBindGroup(BindGroupBinding binding) override;
    void dispatch(DispatchDesc desc) override;
    void end() override;
private:
    std::shared_ptr<MetalCommandBuffer> m_command_buffer;
    id<MTLComputeCommandEncoder> m_encoder = nil;
    std::shared_ptr<const MetalPipeline> m_pipeline;
    bool m_ended = false;
};

} // namespace rhi::metal
