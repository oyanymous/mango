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

/**
 * Sets render encoder state, skipping calls that would set what is already set.
 *
 * Metal API validation (with warnings asserting, as MetalInstance configures it) fails on redundant state calls,
 * e.g. the same cull mode or buffer set twice in a row, which RHI users rebinding per draw would otherwise trigger.
 */
class MetalRenderState {
public:
    /// Wraps @p encoder; the cache starts empty, so the first call of each kind always reaches Metal.
    explicit MetalRenderState(id<MTLRenderCommandEncoder> encoder) : m_encoder(encoder) {}

    void setPipelineState(id<MTLRenderPipelineState> state);                        /**< `setRenderPipelineState:`. */
    void setDepthStencilState(id<MTLDepthStencilState> state);                      /**< `setDepthStencilState:`. */
    void setRaster(MTLCullMode cull, MTLWinding winding, MTLTriangleFillMode fill); /**< Cull, winding, fill. */
    void setStencilReference(std::uint32_t reference);                              /**< `setStencilReferenceValue:`. */
    void setBuffer(ShaderStage stage, id<MTLBuffer> buffer, NSUInteger offset, NSUInteger index); /**< A buffer. */
    void setTexture(ShaderStage stage, id<MTLTexture> texture, NSUInteger index);                 /**< A texture. */
    void setSampler(ShaderStage stage, id<MTLSamplerState> sampler, NSUInteger index);            /**< A sampler. */
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

/**
 * Records one render pass. Ends itself when destroyed.
 *
 * Vertex buffer slot N binds to Metal buffer index `kVertexBufferBase - N`. Draws need a pipeline, and indexed draws
 * an index buffer, set first; otherwise they are skipped. Bind groups bind through the current pipeline's layout, so
 * set the pipeline before them (as in the Vulkan backend).
 */
class MetalRenderEncoder final : public IRenderEncoder {
public:
    /**
     * Wraps an open render encoder. Created by MetalCommandBuffer::beginRender.
     *
     * @param command_buffer The command buffer it records into; retains the resources used.
     * @param extent The render target's extent; scissor rectangles are clamped to it.
     */
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
    std::shared_ptr<const MetalPipeline> m_pipeline; // the current pipeline (primitive type, binding remap)
    id<MTLBuffer> m_index_buffer = nil;              // set by setIndexBuffer, used by drawIndexed
    std::uint64_t m_index_offset = 0;
    MTLIndexType m_index_type = MTLIndexTypeUInt32;
    bool m_ended = false;
    Extent2D m_extent;
    bool m_scissor_empty = false; // the clamped scissor has no area; draws are skipped
    // Viewport and scissor are set on the encoder just before a draw, and only when they changed since the last one.
    MTLViewport m_viewport{};
    MTLScissorRect m_scissor{};
    bool m_viewport_dirty = true;
    bool m_scissor_dirty = true;

    /// Applies a changed viewport or scissor before a draw.
    void flushState();
};

} // namespace rhi::metal
