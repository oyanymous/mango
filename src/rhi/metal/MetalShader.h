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

struct MetalFunction {
    id<MTLFunction> function = nil;
    std::array<std::uint32_t, 3> workgroup_size{1, 1, 1};
};

class MetalShader final : public IShader {
public:
    static Result<ShaderRef> create(std::shared_ptr<MetalDeviceContext> context, ShaderDesc desc);

    MetalShader(std::shared_ptr<MetalDeviceContext> context, ShaderDesc desc);

    ShaderReflection getReflection() const override;

    Result<MetalFunction> compile(const MetalPipelineLayout& layout) const;

    ShaderStage stage() const { return m_desc.stage; }
private:
    ShaderDesc m_desc;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
