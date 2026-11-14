#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

// SPIR-V is the shader input for every backend; non-Vulkan backends would cross-compile it.
struct ShaderDesc {
    ShaderStage stage = ShaderStage::None;
    std::vector<std::uint32_t> spirv;
    std::string entry_point = "main";
    std::string label;
};

// What is known about a shader after creation. Resource-binding reflection may be added later.
struct ShaderReflection {
    ShaderStage stage = ShaderStage::None;
    std::string entry_point;
};

class IShader {
public:
    virtual ~IShader() = default;

    virtual ShaderReflection getReflection() const = 0;
};

} // namespace rhi
