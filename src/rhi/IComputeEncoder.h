#pragma once

#include <cstdint>
#include <vector>

#include "IBindGroup.h"
#include "IPipeline.h"
#include "RHIFwd.h"

namespace rhi {

/// Number of workgroups to launch in each dimension. The workgroup size is set in the shader.
struct DispatchDesc {
    std::uint32_t x = 1; /**< Workgroups along x. */
    std::uint32_t y = 1; /**< Workgroups along y. */
    std::uint32_t z = 1; /**< Workgroups along z. */
};

/**
 * Records compute dispatches. Returned by ICommandBuffer::beginCompute.
 *
 * end() must be called before the command buffer records anything else.
 */
class IComputeEncoder {
public:
    virtual ~IComputeEncoder() = default;

    /**
     * Sets the compute pipeline for following dispatches. Retained by the command buffer.
     *
     * @param pipeline A PipelineKind::Compute pipeline.
     */
    virtual void setPipeline(PipelineRef pipeline) = 0;
    /**
     * Binds a bind group for following dispatches.
     *
     * @param binding Group and the index it binds to.
     */
    virtual void setBindGroup(BindGroupBinding binding) = 0;
    virtual void dispatch(DispatchDesc desc) = 0;
    /// Ends compute recording. The encoder must not be used afterwards.
    virtual void end() = 0;
};

} // namespace rhi
