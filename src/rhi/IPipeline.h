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

enum class StencilOp {
    Keep,
    Zero,
    Replace,
    IncrementClamp,
    DecrementClamp,
    Invert,
    IncrementWrap,
    DecrementWrap,
};

struct StencilFaceState {
    CompareOp compare = CompareOp::Always;
    StencilOp fail_op = StencilOp::Keep;
    StencilOp depth_fail_op = StencilOp::Keep;
    StencilOp pass_op = StencilOp::Keep;
};

struct DepthStencilState {
    bool depth_test = false;
    bool depth_write = false;
    CompareOp depth_compare = CompareOp::Less;
    bool stencil_test = false;
    StencilFaceState stencil;
    std::uint8_t stencil_read_mask = 0xff;
    std::uint8_t stencil_write_mask = 0xff;
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
    std::vector<BlendState> blend;

    RenderPassRef render_pass;
    std::string label;
};

struct ComputePipelineDesc {
    PipelineLayoutRef layout;
    ShaderRef compute_shader;
    std::string label;
};

class IPipeline {
public:
    virtual ~IPipeline() = default;

    virtual PipelineKind getKind() const = 0;
};

} // namespace rhi
