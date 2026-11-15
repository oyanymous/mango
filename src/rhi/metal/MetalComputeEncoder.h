#pragma once

#include <memory>

#include "IComputeEncoder.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalCommandBuffer;
class MetalPipeline;

class MetalComputeEncoder final : public IComputeEncoder {
public:
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
