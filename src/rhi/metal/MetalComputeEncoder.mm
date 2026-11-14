/**
 * @file
 * @brief MetalComputeEncoder: compute recording.
 */

#include "MetalComputeEncoder.h"

#include <utility>

#include "MetalBindGroup.h"
#include "MetalCommandBuffer.h"
#include "MetalPipeline.h"

namespace rhi::metal {

MetalComputeEncoder::MetalComputeEncoder(std::shared_ptr<MetalCommandBuffer> command_buffer,
                                         id<MTLComputeCommandEncoder> encoder)
    : m_command_buffer(std::move(command_buffer)), m_encoder(encoder) {
}

MetalComputeEncoder::~MetalComputeEncoder() {
    end();
}

void MetalComputeEncoder::setPipeline(PipelineRef pipeline) {
    if (m_ended || !pipeline) {
        return;
    }
    const auto& metal = static_cast<const MetalPipeline&>(*pipeline);
    if (metal.getKind() != PipelineKind::Compute) {
        m_command_buffer->reportError("IComputeEncoder::setPipeline: not a compute pipeline");
        return;
    }
    [m_encoder setComputePipelineState:metal.m_compute_state];
    m_pipeline = std::static_pointer_cast<const MetalPipeline>(pipeline);
    m_command_buffer->retain(std::move(pipeline));
}

void MetalComputeEncoder::setBindGroup(BindGroupBinding binding) {
    if (m_ended || !binding.bind_group) {
        return;
    }
    if (!m_pipeline) {
        m_command_buffer->reportError("IComputeEncoder::setBindGroup: set a pipeline first");
        return;
    }
    static_cast<const MetalBindGroup&>(*binding.bind_group).bind(m_encoder, m_pipeline->layout(), binding.index);
    m_command_buffer->retain(std::move(binding.bind_group));
}

void MetalComputeEncoder::dispatch(DispatchDesc desc) {
    if (m_ended || !m_pipeline) {
        return;
    }
    [m_encoder dispatchThreadgroups:MTLSizeMake(desc.x, desc.y, desc.z)
              threadsPerThreadgroup:m_pipeline->m_workgroup_size];
}

void MetalComputeEncoder::end() {
    if (!m_ended) {
        m_ended = true;
        [m_encoder endEncoding];
        m_command_buffer->endEncoder();
    }
}

} // namespace rhi::metal
