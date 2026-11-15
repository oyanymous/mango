#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "IRenderEncoder.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalCommandBuffer;
class MetalPipeline;

class MetalRenderState {
public:
    explicit MetalRenderState(id<MTLRenderCommandEncoder> encoder) : m_encoder(encoder) {}

    void setPipelineState(id<MTLRenderPipelineState> state);
    void setDepthStencilState(id<MTLDepthStencilState> state);
    void setRaster(MTLCullMode cull, MTLWinding winding, MTLTriangleFillMode fill);
    void setStencilReference(std::uint32_t reference);
    void setBuffer(ShaderStage stage, id<MTLBuffer> buffer, NSUInteger offset, NSUInteger index);
    void setTexture(ShaderStage stage, id<MTLTexture> texture, NSUInteger index);
    void setSampler(ShaderStage stage, id<MTLSamplerState> sampler, NSUInteger index);
private:
    static constexpr std::size_t kBuffers = 31;
    static constexpr std::size_t kTextures = 128;
    static constexpr std::size_t kSamplers = 16;
    struct StageState {
        std::array<id<MTLBuffer>, kBuffers> buffers{};
        std::array<NSUInteger, kBuffers> offsets{};
        std::array<id<MTLTexture>, kTextures> textures{};
        std::array<id<MTLSamplerState>, kSamplers> samplers{};
    };
    StageState& stage(ShaderStage stage) { return stage == ShaderStage::Vertex ? m_vertex : m_fragment; }

    id<MTLRenderCommandEncoder> m_encoder = nil;
    id<MTLRenderPipelineState> m_pipeline_state = nil;
    id<MTLDepthStencilState> m_depth_stencil_state = nil;
    bool m_raster_set = false;
    MTLCullMode m_cull = MTLCullModeNone;
    MTLWinding m_winding = MTLWindingClockwise;
    MTLTriangleFillMode m_fill = MTLTriangleFillModeFill;
    bool m_stencil_reference_set = false;
    std::uint32_t m_stencil_reference = 0;
    StageState m_vertex;
    StageState m_fragment;
};

class MetalRenderEncoder final : public IRenderEncoder {
public:
    MetalRenderEncoder(std::shared_ptr<MetalCommandBuffer> command_buffer, id<MTLRenderCommandEncoder> encoder,
                       Extent2D extent);
    ~MetalRenderEncoder() override;

    void setPipeline(PipelineRef pipeline) override;
    void setBindGroup(BindGroupBinding binding) override;
    void setVertexBuffers(VertexBufferBindings bindings) override;
    void setIndexBuffer(IndexBufferBinding binding) override;
    void draw(DrawDesc desc) override;
    void drawIndexed(DrawIndexedDesc desc) override;
    void setViewport(Viewport viewport) override;
    void setScissor(ScissorRect rect) override;
    void setStencilReference(std::uint32_t reference) override;
    void end() override;
private:
    std::shared_ptr<MetalCommandBuffer> m_command_buffer;
    id<MTLRenderCommandEncoder> m_encoder = nil;
    MetalRenderState m_state;
    std::shared_ptr<const MetalPipeline> m_pipeline;
    id<MTLBuffer> m_index_buffer = nil;
    std::uint64_t m_index_offset = 0;
    MTLIndexType m_index_type = MTLIndexTypeUInt32;
    bool m_ended = false;
    Extent2D m_extent;
    bool m_scissor_empty = false;

    MTLViewport m_viewport{};
    MTLScissorRect m_scissor{};
    bool m_viewport_dirty = true;
    bool m_scissor_dirty = true;

    void flushState();
};

} // namespace rhi::metal
