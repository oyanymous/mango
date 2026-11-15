#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "IBindGroupLayout.h"
#include "IBuffer.h"
#include "ISampler.h"
#include "ITextureView.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

struct BindGroupEntry {
    std::uint32_t binding = 0;
    std::uint32_t array_element = 0;
    BufferRef buffer;
    std::uint64_t offset = 0;
    std::uint64_t size = kWholeSize;
    TextureViewRef texture_view;
    SamplerRef sampler;
};

struct BindGroupDesc {
    BindGroupLayoutRef layout;
    std::vector<BindGroupEntry> entries;
    std::string label;
};

struct BindGroupBinding {
    std::uint32_t index = 0;
    BindGroupRef bind_group;
};

class IBindGroup {
public:
    virtual ~IBindGroup() = default;

    virtual BindGroupDesc getDesc() const = 0;
};

} // namespace rhi
