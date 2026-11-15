#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "IBindGroup.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalPipelineLayout;
class MetalRenderState;

struct MetalBoundResource {
    std::uint32_t binding = 0;
    std::uint32_t array_element = 0;
    BindingType type = BindingType::UniformBuffer;
    id<MTLBuffer> buffer = nil;
    std::uint64_t offset = 0;
    id<MTLSamplerState> sampler = nil;

    TextureViewRef texture_view;
};

class MetalBindGroup final : public IBindGroup {
public:
    static Result<BindGroupRef> create(BindGroupDesc desc);

    MetalBindGroup(BindGroupDesc desc, std::vector<MetalBoundResource> resources);

    BindGroupDesc getDesc() const override;

    void bind(MetalRenderState& state, const MetalPipelineLayout& layout, std::uint32_t set) const;

    void bind(id<MTLComputeCommandEncoder> encoder, const MetalPipelineLayout& layout, std::uint32_t set) const;
private:
    BindGroupDesc m_desc;
    std::vector<MetalBoundResource> m_resources;
};

} // namespace rhi::metal
