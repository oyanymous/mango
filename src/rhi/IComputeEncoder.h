#pragma once

/**
 * @file
 * @brief rhi::IComputeEncoder: records compute dispatches.
 */

#include <cstdint>
#include <vector>

#include "IBindGroup.h"
#include "IPipeline.h"
#include "RHIFwd.h"

namespace rhi {

/**
 * @brief Number of workgroups to launch in each dimension. The workgroup size is set in the shader.
 */
struct DispatchDesc {
    std::uint32_t x = 1; /**< Workgroups along x. */
    std::uint32_t y = 1; /**< Workgroups along y. */
    std::uint32_t z = 1; /**< Workgroups along z. */
};

/**
 * @brief Records compute dispatches. Returned by ICommandBuffer::beginCompute.
 *
 * @details
 * end() must be called before the command buffer records anything else.
 */
class IComputeEncoder {
public:
    virtual ~IComputeEncoder() = default;

    /**
     * @brief Sets the compute pipeline for following dispatches. Retained by the command buffer.
     *
     * @param pipeline A PipelineKind::Compute pipeline.
     */
    virtual void setPipeline(PipelineRef pipeline) = 0;
    /**
     * @brief Binds a bind group for following dispatches.
     *
     * @param binding Group and the index it binds to.
     */
    virtual void setBindGroup(BindGroupBinding binding) = 0;
    /**
     * @brief Launches workgroups.
     *
     * @param desc Workgroup counts.
     */
    virtual void dispatch(DispatchDesc desc) = 0;
    /**
     * @brief Ends compute recording. The encoder must not be used afterwards.
     */
    virtual void end() = 0;
};

} // namespace rhi
