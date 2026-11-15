#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "IBindGroupLayout.h"
#include "IPipeline.h"
#include "IPipelineLayout.h"
#include "MetalCommon.h"

namespace rhi::metal {

inline constexpr std::uint32_t kVertexBufferBase = 30;

class MetalBindGroupLayout final : public IBindGroupLayout {
public:
    static Result<BindGroupLayoutRef> create(BindGroupLayoutDesc desc);

    explicit MetalBindGroupLayout(BindGroupLayoutDesc desc);

    BindGroupLayoutDesc getDesc() const override;
private:
    BindGroupLayoutDesc m_desc;
};

struct MetalBindingSlot {
    std::uint32_t binding = 0;
    BindingType type = BindingType::UniformBuffer;
    std::uint32_t count = 1;
    std::uint32_t index = 0;
    ShaderStage visibility = ShaderStage::None;
};

class MetalPipelineLayout final : public IPipelineLayout {
public:
    static Result<PipelineLayoutRef> create(PipelineLayoutDesc desc);

    MetalPipelineLayout(PipelineLayoutDesc desc, std::vector<std::vector<MetalBindingSlot>> sets);

    PipelineLayoutDesc getDesc() const override;

    const std::vector<std::vector<MetalBindingSlot>>& sets() const { return m_sets; }
private:
    PipelineLayoutDesc m_desc;
    std::vector<std::vector<MetalBindingSlot>> m_sets;
};

class MetalPipeline final : public IPipeline {
public:
    static Result<PipelineRef> createGraphics(std::shared_ptr<MetalDeviceContext> context, GraphicsPipelineDesc desc,
                                              bool wireframe_enabled);

    static Result<PipelineRef> createCompute(std::shared_ptr<MetalDeviceContext> context, ComputePipelineDesc desc);

    MetalPipeline(std::shared_ptr<MetalDeviceContext> context, PipelineKind kind, PipelineLayoutRef layout);

    PipelineKind getKind() const override;

    const MetalPipelineLayout& layout() const;

    id<MTLRenderPipelineState> m_render_state = nil;
    id<MTLDepthStencilState> m_depth_stencil_state = nil;
    MTLPrimitiveType m_primitive_type = MTLPrimitiveTypeTriangle;
    MTLCullMode m_cull_mode = MTLCullModeNone;
    MTLWinding m_front_face = MTLWindingCounterClockwise;
    MTLTriangleFillMode m_fill_mode = MTLTriangleFillModeFill;
    id<MTLComputePipelineState> m_compute_state = nil;
    MTLSize m_workgroup_size = MTLSizeMake(1, 1, 1);
private:
    PipelineKind m_kind;
    PipelineLayoutRef m_layout;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
