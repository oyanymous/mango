/**
 * @file
 * @brief MetalBindGroupLayout, MetalPipelineLayout (binding remap) and MetalPipeline creation.
 */

#include "MetalPipeline.h"

#include <algorithm>
#include <set>
#include <string>
#include <utility>

#include "IRenderPass.h"
#include "MetalShader.h"

namespace rhi::metal {

namespace {

/// Argument-table sizes available to bind groups (buffers stop below the vertex buffers).
constexpr std::uint32_t kMaxBindGroupBuffers = kVertexBufferBase + 1 - 8;
constexpr std::uint32_t kMaxTextures = 128; /**< Textures per stage on every Mac2-family GPU. */
constexpr std::uint32_t kMaxSamplers = 16;  /**< Samplers per stage. */

/// The Metal vertex format of an attribute Format, or `MTLVertexFormatInvalid`.
MTLVertexFormat toMTLVertexFormat(Format format) {
    switch (format) {
    case Format::R8Unorm:
        return MTLVertexFormatUCharNormalized;
    case Format::RG8Unorm:
        return MTLVertexFormatUChar2Normalized;
    case Format::RGBA8Unorm:
        return MTLVertexFormatUChar4Normalized;
    case Format::BGRA8Unorm:
        return MTLVertexFormatUChar4Normalized_BGRA;
    case Format::R16Float:
        return MTLVertexFormatHalf;
    case Format::RG16Float:
        return MTLVertexFormatHalf2;
    case Format::RGBA16Float:
        return MTLVertexFormatHalf4;
    case Format::R32Uint:
        return MTLVertexFormatUInt;
    case Format::R32Float:
        return MTLVertexFormatFloat;
    case Format::RG32Float:
        return MTLVertexFormatFloat2;
    case Format::RGB32Float:
        return MTLVertexFormatFloat3;
    case Format::RGBA32Float:
        return MTLVertexFormatFloat4;
    default:
        return MTLVertexFormatInvalid;
    }
}

/// The Metal blend factor of a BlendFactor.
MTLBlendFactor toMTLBlendFactor(BlendFactor factor) {
    switch (factor) {
    case BlendFactor::Zero:
        return MTLBlendFactorZero;
    case BlendFactor::One:
        return MTLBlendFactorOne;
    case BlendFactor::SrcColor:
        return MTLBlendFactorSourceColor;
    case BlendFactor::OneMinusSrcColor:
        return MTLBlendFactorOneMinusSourceColor;
    case BlendFactor::DstColor:
        return MTLBlendFactorDestinationColor;
    case BlendFactor::OneMinusDstColor:
        return MTLBlendFactorOneMinusDestinationColor;
    case BlendFactor::SrcAlpha:
        return MTLBlendFactorSourceAlpha;
    case BlendFactor::OneMinusSrcAlpha:
        return MTLBlendFactorOneMinusSourceAlpha;
    case BlendFactor::DstAlpha:
        return MTLBlendFactorDestinationAlpha;
    case BlendFactor::OneMinusDstAlpha:
        return MTLBlendFactorOneMinusDestinationAlpha;
    }
    return MTLBlendFactorOne;
}

/// The Metal blend operation of a BlendOp.
MTLBlendOperation toMTLBlendOperation(BlendOp op) {
    switch (op) {
    case BlendOp::Add:
        return MTLBlendOperationAdd;
    case BlendOp::Subtract:
        return MTLBlendOperationSubtract;
    case BlendOp::ReverseSubtract:
        return MTLBlendOperationReverseSubtract;
    case BlendOp::Min:
        return MTLBlendOperationMin;
    case BlendOp::Max:
        return MTLBlendOperationMax;
    }
    return MTLBlendOperationAdd;
}

/// The Metal colour write mask of a ColorWriteMask.
MTLColorWriteMask toMTLColorWriteMask(ColorWriteMask mask) {
    MTLColorWriteMask result = MTLColorWriteMaskNone;
    if (hasFlag(mask, ColorWriteMask::Red)) {
        result |= MTLColorWriteMaskRed;
    }
    if (hasFlag(mask, ColorWriteMask::Green)) {
        result |= MTLColorWriteMaskGreen;
    }
    if (hasFlag(mask, ColorWriteMask::Blue)) {
        result |= MTLColorWriteMaskBlue;
    }
    if (hasFlag(mask, ColorWriteMask::Alpha)) {
        result |= MTLColorWriteMaskAlpha;
    }
    return result;
}

/// What draws assemble, and the topology class the pipeline needs, for a PrimitiveTopology.
std::pair<MTLPrimitiveType, MTLPrimitiveTopologyClass> toMTLPrimitive(PrimitiveTopology topology) {
    switch (topology) {
    case PrimitiveTopology::PointList:
        return {MTLPrimitiveTypePoint, MTLPrimitiveTopologyClassPoint};
    case PrimitiveTopology::LineList:
        return {MTLPrimitiveTypeLine, MTLPrimitiveTopologyClassLine};
    case PrimitiveTopology::LineStrip:
        return {MTLPrimitiveTypeLineStrip, MTLPrimitiveTopologyClassLine};
    case PrimitiveTopology::TriangleList:
        return {MTLPrimitiveTypeTriangle, MTLPrimitiveTopologyClassTriangle};
    case PrimitiveTopology::TriangleStrip:
        return {MTLPrimitiveTypeTriangleStrip, MTLPrimitiveTopologyClassTriangle};
    }
    return {MTLPrimitiveTypeTriangle, MTLPrimitiveTopologyClassTriangle};
}

/// Compiles @p shader for @p layout, after checking it is a MetalShader of @p stage.
Result<MetalFunction> compileStage(const ShaderRef& shader, ShaderStage stage, const MetalPipelineLayout& layout) {
    if (!shader) {
        return Error{ErrorCode::InvalidArgument, "pipeline: a required shader is null"};
    }
    const auto& metal_shader = static_cast<const MetalShader&>(*shader);
    if (metal_shader.stage() != stage) {
        return Error{ErrorCode::InvalidArgument, "pipeline: a shader is for another stage"};
    }
    return metal_shader.compile(layout);
}

/// Error from a failed Metal pipeline state creation.
Error pipelineError(const char* what, NSError* error) {
    return Error{ErrorCode::InitializationFailed,
                 std::string(what) + " failed: " + (error ? error.localizedDescription.UTF8String : "unknown error")};
}

/// The Metal stencil operation of a StencilOp.
MTLStencilOperation toMTLStencilOperation(StencilOp op) {
    switch (op) {
    case StencilOp::Keep:
        return MTLStencilOperationKeep;
    case StencilOp::Zero:
        return MTLStencilOperationZero;
    case StencilOp::Replace:
        return MTLStencilOperationReplace;
    case StencilOp::IncrementClamp:
        return MTLStencilOperationIncrementClamp;
    case StencilOp::DecrementClamp:
        return MTLStencilOperationDecrementClamp;
    case StencilOp::Invert:
        return MTLStencilOperationInvert;
    case StencilOp::IncrementWrap:
        return MTLStencilOperationIncrementWrap;
    case StencilOp::DecrementWrap:
        return MTLStencilOperationDecrementWrap;
    }
    return MTLStencilOperationKeep;
}

} // namespace

Result<BindGroupLayoutRef> MetalBindGroupLayout::create(BindGroupLayoutDesc desc) {
    std::set<std::uint32_t> bindings;
    for (const BindGroupLayoutEntry& entry : desc.entries) {
        if (entry.count == 0 || !bindings.insert(entry.binding).second) {
            return Error{ErrorCode::InvalidArgument, "createBindGroupLayout: duplicate binding or zero count"};
        }
    }
    std::ranges::sort(desc.entries, {}, &BindGroupLayoutEntry::binding);
    return BindGroupLayoutRef(std::make_shared<MetalBindGroupLayout>(std::move(desc)));
}

MetalBindGroupLayout::MetalBindGroupLayout(BindGroupLayoutDesc desc) : m_desc(std::move(desc)) {
}

BindGroupLayoutDesc MetalBindGroupLayout::getDesc() const {
    return m_desc;
}

Result<PipelineLayoutRef> MetalPipelineLayout::create(PipelineLayoutDesc desc) {
    std::uint32_t buffers = 0;
    std::uint32_t textures = 0;
    std::uint32_t samplers = 0;
    std::vector<std::vector<MetalBindingSlot>> sets;
    for (const BindGroupLayoutRef& group : desc.bind_group_layouts) {
        if (!group) {
            return Error{ErrorCode::InvalidArgument, "createPipelineLayout: null bind group layout"};
        }
        std::vector<MetalBindingSlot>& slots = sets.emplace_back();
        for (const BindGroupLayoutEntry& entry : group->getDesc().entries) {
            std::uint32_t* next = nullptr;
            switch (entry.type) {
            case BindingType::UniformBuffer:
            case BindingType::StorageBuffer:
                next = &buffers;
                break;
            case BindingType::SampledTexture:
            case BindingType::StorageTexture:
                next = &textures;
                break;
            case BindingType::Sampler:
                next = &samplers;
                break;
            }
            slots.push_back({entry.binding, entry.type, entry.count, *next, entry.visibility});
            *next += entry.count;
        }
    }
    if (buffers > kMaxBindGroupBuffers || textures > kMaxTextures || samplers > kMaxSamplers) {
        return Error{ErrorCode::Unsupported, "createPipelineLayout: the bind groups need more than 22 buffers, 128 "
                                             "textures or 16 samplers per stage"};
    }
    return PipelineLayoutRef(std::make_shared<MetalPipelineLayout>(std::move(desc), std::move(sets)));
}

MetalPipelineLayout::MetalPipelineLayout(PipelineLayoutDesc desc, std::vector<std::vector<MetalBindingSlot>> sets)
    : m_desc(std::move(desc)), m_sets(std::move(sets)) {
}

PipelineLayoutDesc MetalPipelineLayout::getDesc() const {
    return m_desc;
}

Result<PipelineRef> MetalPipeline::createGraphics(std::shared_ptr<MetalDeviceContext> context,
                                                  GraphicsPipelineDesc desc, bool wireframe_enabled) {
    if (!desc.layout || !desc.render_pass) {
        return Error{ErrorCode::InvalidArgument, "createGraphicsPipeline: layout and render_pass are required"};
    }
    if (desc.raster.wireframe && !wireframe_enabled) {
        return Error{ErrorCode::Unsupported,
                     "createGraphicsPipeline: wireframe needs DeviceFeatures::fill_mode_wireframe"};
    }
    if (desc.vertex_buffers.size() > 8) {
        return Error{ErrorCode::Unsupported, "createGraphicsPipeline: more than 8 vertex buffers"};
    }
    const auto& layout = static_cast<const MetalPipelineLayout&>(*desc.layout);

    Result<MetalFunction> vertex = compileStage(desc.vertex_shader, ShaderStage::Vertex, layout);
    if (!vertex) {
        return vertex.error();
    }
    MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
    descriptor.vertexFunction = vertex->function;
    // No fragment shader: a depth-only pipeline.
    if (desc.fragment_shader) {
        Result<MetalFunction> fragment = compileStage(desc.fragment_shader, ShaderStage::Fragment, layout);
        if (!fragment) {
            return fragment.error();
        }
        descriptor.fragmentFunction = fragment->function;
    }

    MTLVertexDescriptor* vertex_descriptor = [MTLVertexDescriptor vertexDescriptor];
    for (std::size_t slot = 0; slot < desc.vertex_buffers.size(); ++slot) {
        const VertexBufferLayout& buffer = desc.vertex_buffers[slot];
        const NSUInteger index = kVertexBufferBase - slot;
        vertex_descriptor.layouts[index].stride = buffer.stride;
        vertex_descriptor.layouts[index].stepFunction = buffer.step_mode == VertexStepMode::Instance
                                                            ? MTLVertexStepFunctionPerInstance
                                                            : MTLVertexStepFunctionPerVertex;
        vertex_descriptor.layouts[index].stepRate = 1;
        for (const VertexAttribute& attribute : buffer.attributes) {
            const MTLVertexFormat format = toMTLVertexFormat(attribute.format);
            if (format == MTLVertexFormatInvalid) {
                return Error{ErrorCode::InvalidArgument, "createGraphicsPipeline: unsupported vertex attribute format"};
            }
            vertex_descriptor.attributes[attribute.location].format = format;
            vertex_descriptor.attributes[attribute.location].offset = attribute.offset;
            vertex_descriptor.attributes[attribute.location].bufferIndex = index;
        }
    }
    descriptor.vertexDescriptor = vertex_descriptor;

    const RenderPassDesc pass = desc.render_pass->getDesc();
    for (std::size_t i = 0; i < pass.color_attachments.size(); ++i) {
        MTLRenderPipelineColorAttachmentDescriptor* color = descriptor.colorAttachments[i];
        color.pixelFormat = toMTLPixelFormat(pass.color_attachments[i].format);
        const BlendState blend = i < desc.blend.size() ? desc.blend[i] : BlendState{};
        color.blendingEnabled = blend.enabled;
        color.sourceRGBBlendFactor = toMTLBlendFactor(blend.src_color);
        color.destinationRGBBlendFactor = toMTLBlendFactor(blend.dst_color);
        color.rgbBlendOperation = toMTLBlendOperation(blend.color_op);
        color.sourceAlphaBlendFactor = toMTLBlendFactor(blend.src_alpha);
        color.destinationAlphaBlendFactor = toMTLBlendFactor(blend.dst_alpha);
        color.alphaBlendOperation = toMTLBlendOperation(blend.alpha_op);
        color.writeMask = toMTLColorWriteMask(blend.write_mask);
    }
    std::uint32_t samples = pass.color_attachments.empty() ? 1 : pass.color_attachments[0].sample_count;
    if (pass.depth_stencil_attachment) {
        const Format depth = pass.depth_stencil_attachment->format;
        descriptor.depthAttachmentPixelFormat = toMTLPixelFormat(depth);
        if (hasStencil(depth)) {
            descriptor.stencilAttachmentPixelFormat = toMTLPixelFormat(depth);
        }
        samples = pass.depth_stencil_attachment->sample_count;
    }
    descriptor.rasterSampleCount = samples;
    const auto [primitive_type, topology_class] = toMTLPrimitive(desc.topology);
    descriptor.inputPrimitiveTopology = topology_class;
    if (!desc.label.empty()) { // a descriptor's label must not be nil
        descriptor.label = toNSString(desc.label);
    }

    NSError* error = nil;
    id<MTLRenderPipelineState> render_state = [context->device newRenderPipelineStateWithDescriptor:descriptor
                                                                                              error:&error];
    if (render_state == nil) {
        return pipelineError("newRenderPipelineStateWithDescriptor", error);
    }

    MTLDepthStencilDescriptor* depth_descriptor = [[MTLDepthStencilDescriptor alloc] init];
    depth_descriptor.depthCompareFunction = desc.depth_stencil.depth_test
                                                ? toMTLCompareFunction(desc.depth_stencil.depth_compare)
                                                : MTLCompareFunctionAlways;
    depth_descriptor.depthWriteEnabled = desc.depth_stencil.depth_test && desc.depth_stencil.depth_write;
    if (desc.depth_stencil.stencil_test && pass.depth_stencil_attachment &&
        hasStencil(pass.depth_stencil_attachment->format)) {
        const StencilFaceState& face = desc.depth_stencil.stencil;
        MTLStencilDescriptor* stencil = [[MTLStencilDescriptor alloc] init];
        stencil.stencilCompareFunction = toMTLCompareFunction(face.compare);
        stencil.stencilFailureOperation = toMTLStencilOperation(face.fail_op);
        stencil.depthFailureOperation = toMTLStencilOperation(face.depth_fail_op);
        stencil.depthStencilPassOperation = toMTLStencilOperation(face.pass_op);
        stencil.readMask = desc.depth_stencil.stencil_read_mask;
        stencil.writeMask = desc.depth_stencil.stencil_write_mask;
        depth_descriptor.frontFaceStencil = stencil;
        depth_descriptor.backFaceStencil = stencil;
    }

    auto pipeline = std::make_shared<MetalPipeline>(std::move(context), PipelineKind::Graphics, desc.layout);
    pipeline->m_render_state = render_state;
    pipeline->m_depth_stencil_state = [pipeline->m_context->device newDepthStencilStateWithDescriptor:depth_descriptor];
    pipeline->m_primitive_type = primitive_type;
    switch (desc.raster.cull_mode) {
    case CullMode::None:
        pipeline->m_cull_mode = MTLCullModeNone;
        break;
    case CullMode::Front:
        pipeline->m_cull_mode = MTLCullModeFront;
        break;
    case CullMode::Back:
        pipeline->m_cull_mode = MTLCullModeBack;
        break;
    }
    // With the flipped vertex Y, window-space winding matches Vulkan's, so the front face maps directly.
    pipeline->m_front_face =
        desc.raster.front_face == FrontFace::Clockwise ? MTLWindingClockwise : MTLWindingCounterClockwise;
    pipeline->m_fill_mode = desc.raster.wireframe ? MTLTriangleFillModeLines : MTLTriangleFillModeFill;
    return PipelineRef(std::move(pipeline));
}

Result<PipelineRef> MetalPipeline::createCompute(std::shared_ptr<MetalDeviceContext> context,
                                                 ComputePipelineDesc desc) {
    if (!desc.layout) {
        return Error{ErrorCode::InvalidArgument, "createComputePipeline: layout is required"};
    }
    const auto& layout = static_cast<const MetalPipelineLayout&>(*desc.layout);
    Result<MetalFunction> compute = compileStage(desc.compute_shader, ShaderStage::Compute, layout);
    if (!compute) {
        return compute.error();
    }

    MTLComputePipelineDescriptor* descriptor = [[MTLComputePipelineDescriptor alloc] init];
    descriptor.computeFunction = compute->function;
    if (!desc.label.empty()) { // a descriptor's label must not be nil
        descriptor.label = toNSString(desc.label);
    }
    NSError* error = nil;
    id<MTLComputePipelineState> state = [context->device newComputePipelineStateWithDescriptor:descriptor
                                                                                       options:MTLPipelineOptionNone
                                                                                    reflection:nil
                                                                                         error:&error];
    if (state == nil) {
        return pipelineError("newComputePipelineStateWithDescriptor", error);
    }
    const auto& size = compute->workgroup_size;
    if (static_cast<NSUInteger>(size[0]) * size[1] * size[2] > state.maxTotalThreadsPerThreadgroup) {
        return Error{ErrorCode::Unsupported, "createComputePipeline: the workgroup is larger than the pipeline allows"};
    }

    auto pipeline = std::make_shared<MetalPipeline>(std::move(context), PipelineKind::Compute, desc.layout);
    pipeline->m_compute_state = state;
    pipeline->m_workgroup_size = MTLSizeMake(size[0], size[1], size[2]);
    return PipelineRef(std::move(pipeline));
}

MetalPipeline::MetalPipeline(std::shared_ptr<MetalDeviceContext> context, PipelineKind kind, PipelineLayoutRef layout)
    : m_kind(kind), m_layout(std::move(layout)), m_context(std::move(context)) {
}

PipelineKind MetalPipeline::getKind() const {
    return m_kind;
}

const MetalPipelineLayout& MetalPipeline::layout() const {
    return static_cast<const MetalPipelineLayout&>(*m_layout);
}

} // namespace rhi::metal
