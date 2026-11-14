#pragma once

/**
 * @file
 * @brief rhi::metal::MetalBindGroupLayout, MetalPipelineLayout and MetalPipeline.
 */

#include <cstdint>
#include <memory>
#include <vector>

#include "IBindGroupLayout.h"
#include "IPipeline.h"
#include "IPipelineLayout.h"
#include "MetalCommon.h"

namespace rhi::metal {

/**
 * @brief Metal buffer argument-table index of vertex buffer slot 0; slot N uses `kVertexBufferBase - N`.
 *
 * @details
 * Vertex buffers and bind-group buffers share the 31 buffer slots of a stage: vertex buffers take the top
 * Limits::max_vertex_buffers (8) from 30 down, bind groups the rest from 0 up.
 */
inline constexpr std::uint32_t kVertexBufferBase = 30;

/**
 * @brief The slots of a bind group. Metal has no layout object; this validates and stores the entries.
 */
class MetalBindGroupLayout final : public IBindGroupLayout {
public:
    /**
     * @brief Validates a layout.
     *
     * @param desc The entries.
     * @return The layout, or InvalidArgument for duplicate binding numbers or a zero count.
     */
    static Result<BindGroupLayoutRef> create(BindGroupLayoutDesc desc);

    /**
     * @brief Wraps a validated layout. Use create(). Entries are sorted by binding number.
     *
     * @param desc The descriptor.
     */
    explicit MetalBindGroupLayout(BindGroupLayoutDesc desc);

    BindGroupLayoutDesc getDesc() const override;
private:
    BindGroupLayoutDesc m_desc;
};

/**
 * @brief Where one binding of a bind group lands in Metal's argument tables.
 */
struct MetalBindingSlot {
    std::uint32_t binding = 0;                     /**< `binding = N` in the shader. */
    BindingType type = BindingType::UniformBuffer; /**< Kind of resource. */
    std::uint32_t count = 1;                       /**< Array size; uses `count` consecutive indices. */
    std::uint32_t index = 0;                       /**< First buffer, texture or sampler index, by type. */
    ShaderStage visibility = ShaderStage::None;    /**< Stages it is bound for. */
};

/**
 * @brief The flat binding remap of a pipeline: bind groups in set order, entries in binding order, each kind of
 * resource counted separately (buffers from 0, textures from 0, samplers from 0). The same indices are used in every
 * stage, so a bind group binds the same way for vertex, fragment and compute.
 */
class MetalPipelineLayout final : public IPipelineLayout {
public:
    /**
     * @brief Assigns argument-table indices.
     *
     * @param desc Bind group layouts, in set order.
     * @return The layout, or Unsupported if the groups need more than 22 buffers (the rest are vertex buffers),
     *         128 textures or 16 samplers.
     */
    static Result<PipelineLayoutRef> create(PipelineLayoutDesc desc);

    /**
     * @brief Wraps assigned slots. Use create().
     *
     * @param desc The descriptor.
     * @param sets Slots per set.
     */
    MetalPipelineLayout(PipelineLayoutDesc desc, std::vector<std::vector<MetalBindingSlot>> sets);

    PipelineLayoutDesc getDesc() const override;

    /**
     * @brief The slots of every set, in set order.
     */
    const std::vector<std::vector<MetalBindingSlot>>& sets() const { return m_sets; }
private:
    PipelineLayoutDesc m_desc;
    std::vector<std::vector<MetalBindingSlot>> m_sets;
};

/**
 * @brief A graphics pipeline (render pipeline state, depth-stencil state and the raster state Metal sets on the
 * encoder) or a compute pipeline (compute pipeline state and workgroup size).
 */
class MetalPipeline final : public IPipeline {
public:
    /**
     * @brief Creates a graphics pipeline.
     *
     * @param context Device context; kept alive by the pipeline.
     * @param desc Shaders, vertex layout, fixed-function state and render pass.
     * @param wireframe_enabled Whether the device was created with DeviceFeatures::fill_mode_wireframe.
     * @return The pipeline, InvalidArgument for missing objects or an unusable vertex format, Unsupported for
     *         wireframe without the feature or too many vertex buffers, or InitializationFailed with the compiler log.
     */
    static Result<PipelineRef> createGraphics(std::shared_ptr<MetalDeviceContext> context, GraphicsPipelineDesc desc,
                                              bool wireframe_enabled);

    /**
     * @brief Creates a compute pipeline.
     *
     * @param context Device context; kept alive by the pipeline.
     * @param desc Shader and layout.
     * @return The pipeline, InvalidArgument for missing objects, or InitializationFailed with the compiler log.
     */
    static Result<PipelineRef> createCompute(std::shared_ptr<MetalDeviceContext> context, ComputePipelineDesc desc);

    /**
     * @brief Creates an empty pipeline of a kind; filled in by createGraphics() / createCompute().
     *
     * @param context Device context.
     * @param kind Graphics or Compute.
     * @param layout The pipeline layout; retained.
     */
    MetalPipeline(std::shared_ptr<MetalDeviceContext> context, PipelineKind kind, PipelineLayoutRef layout);

    PipelineKind getKind() const override;

    /**
     * @brief The binding remap bind groups are set with.
     */
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
