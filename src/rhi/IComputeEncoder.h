#pragma once

#include <cstdint>
#include <vector>

#include "IBindGroup.h"
#include "IPipeline.h"
#include "RHIFwd.h"

namespace rhi {

// Number of workgroups in each dimension.
struct DispatchDesc {
    std::uint32_t x = 1;
    std::uint32_t y = 1;
    std::uint32_t z = 1;
};

// Records compute dispatches. end() must be called before the command buffer records anything else.
class IComputeEncoder {
public:
    virtual ~IComputeEncoder() = default;

    virtual void setPipeline(PipelineRef pipeline) = 0;
    virtual void setBindGroup(BindGroupBinding binding) = 0;
    virtual void dispatch(DispatchDesc desc) = 0;
    virtual void end() = 0;
};

} // namespace rhi
