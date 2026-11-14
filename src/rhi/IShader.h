#pragma once

/// @file
/// rhi::IShader: a compiled shader module.

#include <cstdint>
#include <string>
#include <vector>

#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

/// Describes a shader to create with IDevice::createShader.
///
/// SPIR-V is the shader input for every backend; non-Vulkan backends would cross-compile it.
struct ShaderDesc {
    ShaderStage stage = ShaderStage::None; ///< The single stage this shader is for.
    std::vector<std::uint32_t> spirv;      ///< SPIR-V words (e.g. GLSL compiled at build time).
    std::string entry_point = "main";      ///< Entry function name.
    std::string label;                     ///< Debug name.
};

/// What is known about a shader after creation. Resource-binding reflection may be added later.
struct ShaderReflection {
    ShaderStage stage = ShaderStage::None; ///< The shader's stage.
    std::string entry_point;               ///< Entry function name.
};

/// A shader module for one stage, used to build pipelines.
class IShader {
public:
    virtual ~IShader() = default;

    /// Information about the shader.
    virtual ShaderReflection getReflection() const = 0;
};

} // namespace rhi
