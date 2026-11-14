#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

enum class BindingType {
    UniformBuffer,
    StorageBuffer,
    SampledTexture,
    StorageTexture,
    Sampler,
};

struct BindGroupLayoutEntry {
    std::uint32_t binding = 0;
    BindingType type = BindingType::UniformBuffer;
    ShaderStage visibility = ShaderStage::None;
    std::uint32_t count = 1; // > 1 for arrays
};

// The shape of one bind group (a Vulkan descriptor set): which resource goes in which binding slot.
struct BindGroupLayoutDesc {
    std::vector<BindGroupLayoutEntry> entries;
    std::string label;
};

class IBindGroupLayout {
public:
    virtual ~IBindGroupLayout() = default;

    virtual BindGroupLayoutDesc getDesc() const = 0;
};

} // namespace rhi
