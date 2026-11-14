#pragma once

/// @file
/// rhi::IPipelineLayout: the bind group layouts a pipeline uses.

#include <string>
#include <vector>

#include "IBindGroupLayout.h"
#include "RHIFwd.h"

namespace rhi {

/// Describes a pipeline layout to create with IDevice::createPipelineLayout.
struct PipelineLayoutDesc {
    /// Bind group index `i` in setBindGroup() (`set = i` in the shader) uses `bind_group_layouts[i]`. Retained.
    std::vector<BindGroupLayoutRef> bind_group_layouts;
    std::string label; ///< Debug name.
};

/// The list of bind group layouts a pipeline is compiled against. Retains its bind group layouts.
class IPipelineLayout {
public:
    virtual ~IPipelineLayout() = default;

    /// The descriptor the layout was created with.
    virtual PipelineLayoutDesc getDesc() const = 0;
};

} // namespace rhi
