#pragma once

#include <optional>
#include <string>

#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

enum class Filter {
    Nearest,
    Linear,
};

enum class AddressMode {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
};

struct SamplerDesc {
    Filter min_filter = Filter::Linear;
    Filter mag_filter = Filter::Linear;
    Filter mipmap_filter = Filter::Linear;
    AddressMode address_u = AddressMode::Repeat;
    AddressMode address_v = AddressMode::Repeat;
    AddressMode address_w = AddressMode::Repeat;
    float min_lod = 0.0f;
    float max_lod = 1000.0f;

    float max_anisotropy = 1.0f;
    std::optional<CompareOp> compare;
    std::string label;
};

class ISampler {
public:
    virtual ~ISampler() = default;

    virtual SamplerDesc getDesc() const = 0;
};

} // namespace rhi
