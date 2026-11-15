#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "IShader.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalPipelineLayout;

/// A Metal function compiled from a MetalShader for one pipeline layout.
struct MetalFunction {
    id<MTLFunction> function = nil;
    std::array<std::uint32_t, 3> workgroup_size{1, 1, 1}; /**< Compute only: `local_size_x/y/z`. */
};

/**
 * A shader stage as SPIR-V.
 *
 * Metal compiles Metal Shading Language, and where each resource lands in Metal's argument tables depends on the
 * pipeline layout, so create() only validates the SPIR-V. compile() runs SPIRV-Cross with the layout's binding
 * remap (MetalPipelineLayout) and compiles the MSL with `newLibraryWithSource:`; pipelines call it once each.
 * Vertex shaders are emitted with Y flipped, so clip space matches Vulkan's (+Y down) and the same SPIR-V draws the
 * same image on both backends.
 */
class MetalShader final : public IShader {
public:
    /**
     * Validates SPIR-V and its entry point.
     *
     * @return The shader, or InvalidArgument for empty or malformed SPIR-V, a stage that is not exactly one of
     *         Vertex, Fragment or Compute, or a missing entry point for that stage.
     */
    static Result<ShaderRef> create(std::shared_ptr<MetalDeviceContext> context, ShaderDesc desc);

    /// Wraps validated SPIR-V. Use create().
    MetalShader(std::shared_ptr<MetalDeviceContext> context, ShaderDesc desc);

    ShaderReflection getReflection() const override;

    /**
     * Translates to MSL with @p layout's bindings and compiles it.
     *
     * @param layout Binding remap of the pipeline the function is for.
     * @return The function, or InitializationFailed with the SPIRV-Cross or Metal compiler message.
     */
    Result<MetalFunction> compile(const MetalPipelineLayout& layout) const;

    ShaderStage stage() const { return m_desc.stage; }
private:
    ShaderDesc m_desc;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
