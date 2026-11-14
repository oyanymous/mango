#pragma once

/**
 * @file
 * @brief rhi::IPipeline: compiled graphics and compute pipelines and their fixed-function state.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "IPipelineLayout.h"
#include "IRenderPass.h"
#include "IShader.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

/**
 * @brief Whether a pipeline draws or dispatches.
 */
enum class PipelineKind {
    Graphics, /**< Created by IDevice::createGraphicsPipeline; used with IRenderEncoder. */
    Compute,  /**< Created by IDevice::createComputePipeline; used with IComputeEncoder. */
};

/**
 * @brief How often a vertex buffer advances.
 */
enum class VertexStepMode {
    Vertex,   /**< Once per vertex. */
    Instance, /**< Once per instance. */
};

/**
 * @brief One vertex shader input read from a vertex buffer.
 */
struct VertexAttribute {
    std::uint32_t location = 0;        /**< Shader input location (`layout(location = N)`). */
    Format format = Format::Undefined; /**< Data format, e.g. RGB32Float for a vec3. */
    std::uint32_t offset = 0;          /**< Byte offset within one vertex. */
};

/**
 * @brief The layout of one vertex buffer slot. Slot `i` is bound by setVertexBuffers() at index `i`.
 */
struct VertexBufferLayout {
    std::uint32_t stride = 0;                          /**< Bytes between consecutive elements. */
    VertexStepMode step_mode = VertexStepMode::Vertex; /**< Per-vertex or per-instance. */
    std::vector<VertexAttribute> attributes;           /**< Attributes read from this buffer. */
};

/**
 * @brief How vertices are assembled into primitives.
 */
enum class PrimitiveTopology {
    PointList,     /**< Each vertex is a point. */
    LineList,      /**< Each pair of vertices is a line. */
    LineStrip,     /**< Consecutive vertices form a connected line. */
    TriangleList,  /**< Each three vertices are a triangle. */
    TriangleStrip, /**< Each vertex after the first two forms a triangle with the previous two. */
};

/**
 * @brief Which triangle faces are discarded.
 */
enum class CullMode {
    None,  /**< Draw both faces. */
    Front, /**< Discard front faces. */
    Back,  /**< Discard back faces. */
};

/**
 * @brief Which winding order is the front face.
 */
enum class FrontFace {
    CounterClockwise, /**< Counter-clockwise triangles face the viewer. */
    Clockwise,        /**< Clockwise triangles face the viewer. */
};

/**
 * @brief Rasterizer state.
 */
struct RasterState {
    CullMode cull_mode = CullMode::None;                /**< Face culling. */
    FrontFace front_face = FrontFace::CounterClockwise; /**< Front-face winding. */
    bool wireframe = false; /**< Draw edges only (needs DeviceFeatures::fill_mode_wireframe). */
};

/**
 * @brief Depth test state.
 */
struct DepthStencilState {
    bool depth_test = false;                   /**< Compare fragment depth against the depth buffer. */
    bool depth_write = false;                  /**< Write passing fragments' depth. */
    CompareOp depth_compare = CompareOp::Less; /**< Comparison used by the depth test. */
};

/**
 * @brief A blend factor: what a colour is multiplied by before blending.
 */
enum class BlendFactor {
    Zero,             /**< 0. */
    One,              /**< 1. */
    SrcColor,         /**< Source (fragment) colour. */
    OneMinusSrcColor, /**< 1 - source colour. */
    DstColor,         /**< Destination (attachment) colour. */
    OneMinusDstColor, /**< 1 - destination colour. */
    SrcAlpha,         /**< Source alpha. */
    OneMinusSrcAlpha, /**< 1 - source alpha. */
    DstAlpha,         /**< Destination alpha. */
    OneMinusDstAlpha, /**< 1 - destination alpha. */
};

/**
 * @brief How the weighted source and destination are combined.
 */
enum class BlendOp {
    Add,             /**< src + dst. */
    Subtract,        /**< src - dst. */
    ReverseSubtract, /**< dst - src. */
    Min,             /**< min(src, dst). */
    Max,             /**< max(src, dst). */
};

/**
 * @brief Which colour channels are written, as a bit set.
 */
enum class ColorWriteMask : std::uint32_t {
    None = 0,                         /**< Write nothing. */
    Red = 1u << 0,                    /**< Red. */
    Green = 1u << 1,                  /**< Green. */
    Blue = 1u << 2,                   /**< Blue. */
    Alpha = 1u << 3,                  /**< Alpha. */
    All = Red | Green | Blue | Alpha, /**< Every channel. */
};
MANGO_RHI_FLAGS(ColorWriteMask)

/**
 * @brief Blending for one colour attachment. The defaults disable blending (result = source).
 *
 * @details
 * Typical alpha blending: `src_color = SrcAlpha`, `dst_color = OneMinusSrcAlpha`.
 */
struct BlendState {
    bool enabled = false;                            /**< Blend; when false the fragment colour is written as is. */
    BlendFactor src_color = BlendFactor::One;        /**< Source colour factor. */
    BlendFactor dst_color = BlendFactor::Zero;       /**< Destination colour factor. */
    BlendOp color_op = BlendOp::Add;                 /**< Colour combine operation. */
    BlendFactor src_alpha = BlendFactor::One;        /**< Source alpha factor. */
    BlendFactor dst_alpha = BlendFactor::Zero;       /**< Destination alpha factor. */
    BlendOp alpha_op = BlendOp::Add;                 /**< Alpha combine operation. */
    ColorWriteMask write_mask = ColorWriteMask::All; /**< Channels written. */
};

/**
 * @brief Describes a graphics pipeline to create with IDevice::createGraphicsPipeline.
 */
struct GraphicsPipelineDesc {
    PipelineLayoutRef layout;                                     /**< Bind group layouts the shaders use; retained. */
    ShaderRef vertex_shader;                                      /**< Vertex stage; retained. */
    ShaderRef fragment_shader;                                    /**< Fragment stage; retained. */
    std::vector<VertexBufferLayout> vertex_buffers;               /**< One layout per vertex buffer slot. */
    PrimitiveTopology topology = PrimitiveTopology::TriangleList; /**< Primitive assembly. */
    RasterState raster;                                           /**< Rasterizer state. */
    DepthStencilState depth_stencil;                              /**< Depth test state. */
    std::vector<BlendState> blend; /**< One per colour attachment of `render_pass`; empty means no blending. */
    /**
     * @brief The pipeline works with any render target whose pass is compatible with this one.
     */
    RenderPassRef render_pass;
    std::string label; /**< Debug name. */
};

/**
 * @brief Describes a compute pipeline to create with IDevice::createComputePipeline.
 */
struct ComputePipelineDesc {
    PipelineLayoutRef layout; /**< Bind group layouts the shader uses; retained. */
    ShaderRef compute_shader; /**< Compute stage; retained. */
    std::string label;        /**< Debug name. */
};

/**
 * @brief A compiled graphics or compute pipeline. Retains its layout and shaders.
 */
class IPipeline {
public:
    virtual ~IPipeline() = default;

    /**
     * @brief Whether this is a graphics or a compute pipeline.
     */
    virtual PipelineKind getKind() const = 0;
};

} // namespace rhi
