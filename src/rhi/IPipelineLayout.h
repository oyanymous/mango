#pragma once

#include <string>
#include <vector>

#include "IBindGroupLayout.h"
#include "RHIFwd.h"

namespace rhi {

struct PipelineLayoutDesc {
    std::vector<BindGroupLayoutRef> bind_group_layouts;
    std::string label;
};

class IPipelineLayout {
public:
    virtual ~IPipelineLayout() = default;

    virtual PipelineLayoutDesc getDesc() const = 0;
};

} // namespace rhi
