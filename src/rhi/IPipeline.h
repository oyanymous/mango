#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "IPipelineLayout.h"
#include "IRenderPass.h"
#include "IShader.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

enum class PipelineKind {
    Graphics,
    Compute,
};

enum class VertexStepMode {
    Vertex,
    Instance,
};

struct VertexAttribute {
    std::uint32_t location = 0;
    Format format = Format::Undefined;
    std::uint32_t offset = 0;
};

// One vertex buffer slot; slot i is bound by setVertexBuffers() at index i.
struct VertexBufferLayout {
    std::uint32_t stride = 0;
    VertexStepMode step_mode = VertexStepMode::Vertex;
    std::vector<VertexAttribute> attributes;
};

enum class PrimitiveTopology {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
};

enum class CullMode {
    None,
    Front,
    Back,
};

enum class FrontFace {
    CounterClockwise,
    Clockwise,
};

struct RasterState {
    CullMode cull_mode = CullMode::None;
    FrontFace front_face = FrontFace::CounterClockwise;
    bool wireframe = false;
};

struct DepthStencilState {
    bool depth_test = false;
    bool depth_write = false;
    CompareOp depth_compare = CompareOp::Less;
};

enum class BlendFactor {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
};

enum class BlendOp {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

enum class ColorWriteMask : std::uint32_t {
    None = 0,
    Red = 1u << 0,
    Green = 1u << 1,
    Blue = 1u << 2,
    Alpha = 1u << 3,
    All = Red | Green | Blue | Alpha,
};
MANGO_RHI_FLAGS(ColorWriteMask)

// Blending for one colour attachment.
struct BlendState {
    bool enabled = false;
    BlendFactor src_color = BlendFactor::One;
    BlendFactor dst_color = BlendFactor::Zero;
    BlendOp color_op = BlendOp::Add;
    BlendFactor src_alpha = BlendFactor::One;
    BlendFactor dst_alpha = BlendFactor::Zero;
    BlendOp alpha_op = BlendOp::Add;
    ColorWriteMask write_mask = ColorWriteMask::All;
};

struct GraphicsPipelineDesc {
    PipelineLayoutRef layout;
    ShaderRef vertex_shader;
    ShaderRef fragment_shader;
    std::vector<VertexBufferLayout> vertex_buffers;
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    RasterState raster;
    DepthStencilState depth_stencil;
    std::vector<BlendState> blend; // one per colour attachment of render_pass; empty = no blending
    // The pipeline can be used with any render target whose pass is compatible with this one.
    RenderPassRef render_pass;
    std::string label;
};

struct ComputePipelineDesc {
    PipelineLayoutRef layout;
    ShaderRef compute_shader;
    std::string label;
};

// A compiled graphics or compute pipeline. Retains its layout and shaders.
class IPipeline {
public:
    virtual ~IPipeline() = default;

    virtual PipelineKind getKind() const = 0;
};

} // namespace rhi
