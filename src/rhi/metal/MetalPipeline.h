#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "IBindGroupLayout.h"
#include "IPipeline.h"
#include "IPipelineLayout.h"
#include "MetalCommon.h"

namespace rhi::metal {

/**
 * Metal buffer argument-table index of vertex buffer slot 0; slot N uses `kVertexBufferBase - N`.
 *
 * Vertex buffers and bind-group buffers share the 31 buffer slots of a stage: vertex buffers take the top
 * Limits::max_vertex_buffers (8) from 30 down, bind groups the rest from 0 up.
 */
inline constexpr std::uint32_t kVertexBufferBase = 30;

/// The slots of a bind group. Metal has no layout object; this validates and stores the entries.
class MetalBindGroupLayout final : public IBindGroupLayout {
public:
    /**
     * Validates a layout.
     *
     * @return The layout, or InvalidArgument for duplicate binding numbers or a zero count.
     */
    static Result<BindGroupLayoutRef> create(BindGroupLayoutDesc desc);

    /// Wraps a validated layout. Use create(). Entries are sorted by binding number.
    explicit MetalBindGroupLayout(BindGroupLayoutDesc desc);

    BindGroupLayoutDesc getDesc() const override;
private:
    BindGroupLayoutDesc m_desc;
};

/// Where one binding of a bind group lands in Metal's argument tables.
struct MetalBindingSlot {
    std::uint32_t binding = 0; /**< `binding = N` in the shader. */
    BindingType type = BindingType::UniformBuffer;
    std::uint32_t count = 1;                    /**< Array size; uses `count` consecutive indices. */
    std::uint32_t index = 0;                    /**< First buffer, texture or sampler index, by type. */
    ShaderStage visibility = ShaderStage::None; /**< Stages it is bound for. */
};

/// The flat binding remap of a pipeline: bind groups in set order, entries in binding order, each kind of resource
/// counted separately (buffers from 0, textures from 0, samplers from 0). The same indices are used in every stage, so
/// a bind group binds the same way for vertex, fragment and compute.
class MetalPipelineLayout final : public IPipelineLayout {
public:
    /**
     * Assigns argument-table indices.
     *
     * @param desc Bind group layouts, in set order.
     * @return The layout, or Unsupported if the groups need more than 22 buffers (the rest are vertex buffers),
     *         128 textures or 16 samplers.
     */
    static Result<PipelineLayoutRef> create(PipelineLayoutDesc desc);

    /**
     * Wraps assigned slots. Use create().
     *
     * @param sets Slots per set.
     */
    MetalPipelineLayout(PipelineLayoutDesc desc, std::vector<std::vector<MetalBindingSlot>> sets);

    PipelineLayoutDesc getDesc() const override;

    /// The slots of every set, in set order.
    const std::vector<std::vector<MetalBindingSlot>>& sets() const { return m_sets; }
private:
    PipelineLayoutDesc m_desc;
    std::vector<std::vector<MetalBindingSlot>> m_sets;
};

/// A graphics pipeline (render pipeline state, depth-stencil state and the raster state Metal sets on the encoder) or a
/// compute pipeline (compute pipeline state and workgroup size).
class MetalPipeline final : public IPipeline {
public:
    /**
     * Creates a graphics pipeline.
     *
     * @param wireframe_enabled Whether the device was created with DeviceFeatures::fill_mode_wireframe.
     * @return The pipeline, InvalidArgument for missing objects or an unusable vertex format, Unsupported for
     *         wireframe without the feature or too many vertex buffers, or InitializationFailed with the compiler log.
     */
    static Result<PipelineRef> createGraphics(std::shared_ptr<MetalDeviceContext> context, GraphicsPipelineDesc desc,
                                              bool wireframe_enabled);

    /**
     * Creates a compute pipeline.
     *
     * @return The pipeline, InvalidArgument for missing objects, or InitializationFailed with the compiler log.
     */
    static Result<PipelineRef> createCompute(std::shared_ptr<MetalDeviceContext> context, ComputePipelineDesc desc);

    /**
     * Creates an empty pipeline of a kind; filled in by createGraphics() / createCompute().
     *
     * @param layout The pipeline layout; retained.
     */
    MetalPipeline(std::shared_ptr<MetalDeviceContext> context, PipelineKind kind, PipelineLayoutRef layout);

    PipelineKind getKind() const override;

    /// The binding remap bind groups are set with.
    const MetalPipelineLayout& layout() const;

    id<MTLRenderPipelineState> m_render_state = nil;              /**< Graphics: the render pipeline state. */
    id<MTLDepthStencilState> m_depth_stencil_state = nil;         /**< Graphics: depth test state. */
    MTLPrimitiveType m_primitive_type = MTLPrimitiveTypeTriangle; /**< Graphics: what draws assemble. */
    MTLCullMode m_cull_mode = MTLCullModeNone;                    /**< Graphics: face culling. */
    MTLWinding m_front_face = MTLWindingCounterClockwise;         /**< Graphics: front-face winding. */
    MTLTriangleFillMode m_fill_mode = MTLTriangleFillModeFill;    /**< Graphics: fill or wireframe. */
    id<MTLComputePipelineState> m_compute_state = nil;            /**< Compute: the compute pipeline state. */
    MTLSize m_workgroup_size = MTLSizeMake(1, 1, 1);              /**< Compute: threads per threadgroup. */
private:
    PipelineKind m_kind;
    PipelineLayoutRef m_layout;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
