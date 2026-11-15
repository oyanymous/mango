/**
 * @file
 * @brief VulkanShader, VulkanPipelineLayout and VulkanPipeline.
 */

#include "VulkanPipeline.h"

#include <array>
#include <vector>

#include "VulkanBindGroup.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"

namespace rhi::vulkan {

namespace {

vk::PrimitiveTopology toVkTopology(PrimitiveTopology topology) {
    switch (topology) {
    case PrimitiveTopology::PointList:
        return vk::PrimitiveTopology::ePointList;
    case PrimitiveTopology::LineList:
        return vk::PrimitiveTopology::eLineList;
    case PrimitiveTopology::LineStrip:
        return vk::PrimitiveTopology::eLineStrip;
    case PrimitiveTopology::TriangleList:
        return vk::PrimitiveTopology::eTriangleList;
    case PrimitiveTopology::TriangleStrip:
        return vk::PrimitiveTopology::eTriangleStrip;
    }
    return vk::PrimitiveTopology::eTriangleList;
}

vk::CullModeFlags toVkCullMode(CullMode mode) {
    switch (mode) {
    case CullMode::None:
        return vk::CullModeFlagBits::eNone;
    case CullMode::Front:
        return vk::CullModeFlagBits::eFront;
    case CullMode::Back:
        return vk::CullModeFlagBits::eBack;
    }
    return vk::CullModeFlagBits::eNone;
}

/// Maps a BlendFactor; the enumerators are declared in Vulkan's order.
vk::BlendFactor toVkBlendFactor(BlendFactor factor) {
    static_assert(static_cast<int>(BlendFactor::OneMinusDstAlpha) == VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA);
    return static_cast<vk::BlendFactor>(factor);
}

/// Maps a BlendOp; the enumerators are declared in Vulkan's order.
vk::BlendOp toVkBlendOp(BlendOp op) {
    static_assert(static_cast<int>(BlendOp::Max) == VK_BLEND_OP_MAX);
    return static_cast<vk::BlendOp>(op);
}

vk::PipelineColorBlendAttachmentState toVkBlend(const BlendState& blend) {
    return vk::PipelineColorBlendAttachmentState{
        .blendEnable = blend.enabled ? vk::True : vk::False,
        .srcColorBlendFactor = toVkBlendFactor(blend.src_color),
        .dstColorBlendFactor = toVkBlendFactor(blend.dst_color),
        .colorBlendOp = toVkBlendOp(blend.color_op),
        .srcAlphaBlendFactor = toVkBlendFactor(blend.src_alpha),
        .dstAlphaBlendFactor = toVkBlendFactor(blend.dst_alpha),
        .alphaBlendOp = toVkBlendOp(blend.alpha_op),
        // ColorWriteMask uses Vulkan's R, G, B, A bit positions.
        .colorWriteMask = vk::ColorComponentFlags(static_cast<VkColorComponentFlags>(blend.write_mask)),
    };
}

/// Whether @p format has a stencil plane.
bool hasStencil(Format format) {
    return format == Format::D24UnormS8Uint || format == Format::D32FloatS8Uint;
}

vk::StencilOp toVkStencilOp(StencilOp op) {
    switch (op) {
    case StencilOp::Keep:
        return vk::StencilOp::eKeep;
    case StencilOp::Zero:
        return vk::StencilOp::eZero;
    case StencilOp::Replace:
        return vk::StencilOp::eReplace;
    case StencilOp::IncrementClamp:
        return vk::StencilOp::eIncrementAndClamp;
    case StencilOp::DecrementClamp:
        return vk::StencilOp::eDecrementAndClamp;
    case StencilOp::Invert:
        return vk::StencilOp::eInvert;
    case StencilOp::IncrementWrap:
        return vk::StencilOp::eIncrementAndWrap;
    case StencilOp::DecrementWrap:
        return vk::StencilOp::eDecrementAndWrap;
    }
    return vk::StencilOp::eKeep;
}

/// The shader stage create info for @p shader, which must be a VulkanShader.
vk::PipelineShaderStageCreateInfo toVkStage(const ShaderRef& shader, vk::ShaderStageFlagBits stage,
                                            const std::string& entry_point) {
    return vk::PipelineShaderStageCreateInfo{
        .stage = stage,
        .module = static_cast<const VulkanShader&>(*shader).handle(),
        .pName = entry_point.c_str(),
    };
}

} // namespace

// --- VulkanShader ---

Result<ShaderRef> VulkanShader::create(std::shared_ptr<VulkanDeviceHandle> device, ShaderDesc desc) {
    if (desc.spirv.empty()) {
        return Error{ErrorCode::InvalidArgument, "createShader: spirv is empty"};
    }
    if (desc.stage != ShaderStage::Vertex && desc.stage != ShaderStage::Fragment &&
        desc.stage != ShaderStage::Compute) {
        return Error{ErrorCode::InvalidArgument,
                     "createShader: stage must be exactly one of Vertex, Fragment, Compute"};
    }

    const vk::ShaderModuleCreateInfo info{
        .codeSize = desc.spirv.size() * sizeof(std::uint32_t),
        .pCode = desc.spirv.data(),
    };
    Result<vk::raii::ShaderModule> module =
        check(device->m_device.createShaderModule(info, nullptr), "vkCreateShaderModule");
    if (!module) {
        return module.error();
    }
    return ShaderRef(std::make_shared<VulkanShader>(
        std::move(device), ShaderReflection{.stage = desc.stage, .entry_point = std::move(desc.entry_point)},
        std::move(module).value()));
}

VulkanShader::VulkanShader(std::shared_ptr<VulkanDeviceHandle> device, ShaderReflection reflection,
                           vk::raii::ShaderModule module)
    : m_device(std::move(device)), m_reflection(std::move(reflection)), m_module(std::move(module)) {
}

ShaderReflection VulkanShader::getReflection() const {
    return m_reflection;
}

// --- VulkanPipelineLayout ---

Result<PipelineLayoutRef> VulkanPipelineLayout::create(std::shared_ptr<VulkanDeviceHandle> device,
                                                       PipelineLayoutDesc desc) {
    std::vector<vk::DescriptorSetLayout> set_layouts;
    for (const BindGroupLayoutRef& layout : desc.bind_group_layouts) {
        if (!layout) {
            return Error{ErrorCode::InvalidArgument, "createPipelineLayout: a bind group layout is null"};
        }
        set_layouts.push_back(static_cast<const VulkanBindGroupLayout&>(*layout).handle());
    }

    const vk::PipelineLayoutCreateInfo info{
        .setLayoutCount = static_cast<std::uint32_t>(set_layouts.size()),
        .pSetLayouts = set_layouts.data(),
    };
    Result<vk::raii::PipelineLayout> layout =
        check(device->m_device.createPipelineLayout(info, nullptr), "vkCreatePipelineLayout");
    if (!layout) {
        return layout.error();
    }
    return PipelineLayoutRef(
        std::make_shared<VulkanPipelineLayout>(std::move(device), std::move(desc), std::move(layout).value()));
}

VulkanPipelineLayout::VulkanPipelineLayout(std::shared_ptr<VulkanDeviceHandle> device, PipelineLayoutDesc desc,
                                           vk::raii::PipelineLayout layout)
    : m_device(std::move(device)), m_desc(std::move(desc)), m_layout(std::move(layout)) {
}

PipelineLayoutDesc VulkanPipelineLayout::getDesc() const {
    return m_desc;
}

// --- VulkanPipeline ---

Result<PipelineRef> VulkanPipeline::createGraphics(std::shared_ptr<VulkanDeviceHandle> device,
                                                   GraphicsPipelineDesc desc, bool wireframe_enabled) {
    if (!desc.layout || !desc.vertex_shader || !desc.fragment_shader || !desc.render_pass) {
        return Error{ErrorCode::InvalidArgument,
                     "createGraphicsPipeline: layout, vertex_shader, fragment_shader and render_pass are required"};
    }
    const ShaderReflection vertex = desc.vertex_shader->getReflection();
    const ShaderReflection fragment = desc.fragment_shader->getReflection();
    if (vertex.stage != ShaderStage::Vertex || fragment.stage != ShaderStage::Fragment) {
        return Error{ErrorCode::InvalidArgument, "createGraphicsPipeline: a shader has the wrong stage"};
    }
    const RenderPassDesc pass = desc.render_pass->getDesc();
    if (!desc.blend.empty() && desc.blend.size() != pass.color_attachments.size()) {
        return Error{ErrorCode::InvalidArgument,
                     "createGraphicsPipeline: blend needs one entry per colour attachment of the render pass"};
    }
    if (desc.raster.wireframe && !wireframe_enabled) {
        return Error{ErrorCode::Unsupported,
                     "createGraphicsPipeline: wireframe needs DeviceFeatures::fill_mode_wireframe"};
    }

    const std::array<vk::PipelineShaderStageCreateInfo, 2> stages = {
        toVkStage(desc.vertex_shader, vk::ShaderStageFlagBits::eVertex, vertex.entry_point),
        toVkStage(desc.fragment_shader, vk::ShaderStageFlagBits::eFragment, fragment.entry_point),
    };

    std::vector<vk::VertexInputBindingDescription> bindings;
    std::vector<vk::VertexInputAttributeDescription> attributes;
    for (std::uint32_t slot = 0; slot < desc.vertex_buffers.size(); ++slot) {
        const VertexBufferLayout& layout = desc.vertex_buffers[slot];
        bindings.push_back({
            .binding = slot,
            .stride = layout.stride,
            .inputRate = layout.step_mode == VertexStepMode::Instance ? vk::VertexInputRate::eInstance
                                                                      : vk::VertexInputRate::eVertex,
        });
        for (const VertexAttribute& attribute : layout.attributes) {
            attributes.push_back({
                .location = attribute.location,
                .binding = slot,
                .format = toVkFormat(attribute.format),
                .offset = attribute.offset,
            });
        }
    }
    const vk::PipelineVertexInputStateCreateInfo vertex_input{
        .vertexBindingDescriptionCount = static_cast<std::uint32_t>(bindings.size()),
        .pVertexBindingDescriptions = bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size()),
        .pVertexAttributeDescriptions = attributes.data(),
    };

    const vk::PipelineInputAssemblyStateCreateInfo input_assembly{.topology = toVkTopology(desc.topology)};

    // Viewport and scissor are dynamic; only their counts are fixed here.
    const vk::PipelineViewportStateCreateInfo viewport{.viewportCount = 1, .scissorCount = 1};

    const vk::PipelineRasterizationStateCreateInfo raster{
        .polygonMode = desc.raster.wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill,
        .cullMode = toVkCullMode(desc.raster.cull_mode),
        .frontFace = desc.raster.front_face == FrontFace::Clockwise ? vk::FrontFace::eClockwise
                                                                    : vk::FrontFace::eCounterClockwise,
        .lineWidth = 1.0f,
    };

    // Every attachment of a pass has the same sample count.
    const std::uint32_t samples = !pass.color_attachments.empty() ? pass.color_attachments[0].sample_count
                                                                  : pass.depth_stencil_attachment->sample_count;
    const vk::PipelineMultisampleStateCreateInfo multisample{
        .rasterizationSamples = static_cast<vk::SampleCountFlagBits>(samples),
    };

    vk::PipelineDepthStencilStateCreateInfo depth{
        .depthTestEnable = desc.depth_stencil.depth_test ? vk::True : vk::False,
        .depthWriteEnable = desc.depth_stencil.depth_write ? vk::True : vk::False,
        .depthCompareOp = toVkCompareOp(desc.depth_stencil.depth_compare),
    };
    if (desc.depth_stencil.stencil_test && pass.depth_stencil_attachment &&
        hasStencil(pass.depth_stencil_attachment->format)) {
        const StencilFaceState& face = desc.depth_stencil.stencil;
        const vk::StencilOpState state{
            .failOp = toVkStencilOp(face.fail_op),
            .passOp = toVkStencilOp(face.pass_op),
            .depthFailOp = toVkStencilOp(face.depth_fail_op),
            .compareOp = toVkCompareOp(face.compare),
            .compareMask = desc.depth_stencil.stencil_read_mask,
            .writeMask = desc.depth_stencil.stencil_write_mask,
        };
        depth.stencilTestEnable = vk::True;
        depth.front = state;
        depth.back = state;
    }

    std::vector<vk::PipelineColorBlendAttachmentState> blends;
    for (std::size_t i = 0; i < pass.color_attachments.size(); ++i) {
        blends.push_back(toVkBlend(desc.blend.empty() ? BlendState{} : desc.blend[i]));
    }
    const vk::PipelineColorBlendStateCreateInfo blend{
        .attachmentCount = static_cast<std::uint32_t>(blends.size()),
        .pAttachments = blends.data(),
    };

    const std::array<vk::DynamicState, 3> dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor,
                                                            vk::DynamicState::eStencilReference};
    const vk::PipelineDynamicStateCreateInfo dynamic{
        .dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    const vk::GraphicsPipelineCreateInfo info{
        .stageCount = static_cast<std::uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport,
        .pRasterizationState = &raster,
        .pMultisampleState = &multisample,
        .pDepthStencilState = pass.depth_stencil_attachment ? &depth : nullptr,
        .pColorBlendState = &blend,
        .pDynamicState = &dynamic,
        .layout = static_cast<const VulkanPipelineLayout&>(*desc.layout).handle(),
        .renderPass = static_cast<const VulkanRenderPass&>(*desc.render_pass).handle(),
        .subpass = 0,
    };

    Result<vk::raii::Pipeline> pipeline =
        check(device->m_device.createGraphicsPipeline(nullptr, info, nullptr), "vkCreateGraphicsPipelines");
    if (!pipeline) {
        return pipeline.error();
    }
    return PipelineRef(std::make_shared<VulkanPipeline>(
        std::move(device), PipelineKind::Graphics, std::move(desc.layout),
        std::vector<ShaderRef>{std::move(desc.vertex_shader), std::move(desc.fragment_shader)},
        std::move(pipeline).value()));
}

Result<PipelineRef> VulkanPipeline::createCompute(std::shared_ptr<VulkanDeviceHandle> device,
                                                  ComputePipelineDesc desc) {
    if (!desc.layout || !desc.compute_shader) {
        return Error{ErrorCode::InvalidArgument, "createComputePipeline: layout and compute_shader are required"};
    }
    const ShaderReflection compute = desc.compute_shader->getReflection();
    if (compute.stage != ShaderStage::Compute) {
        return Error{ErrorCode::InvalidArgument, "createComputePipeline: the shader is not a compute shader"};
    }

    const vk::ComputePipelineCreateInfo info{
        .stage = toVkStage(desc.compute_shader, vk::ShaderStageFlagBits::eCompute, compute.entry_point),
        .layout = static_cast<const VulkanPipelineLayout&>(*desc.layout).handle(),
    };
    Result<vk::raii::Pipeline> pipeline =
        check(device->m_device.createComputePipeline(nullptr, info, nullptr), "vkCreateComputePipelines");
    if (!pipeline) {
        return pipeline.error();
    }
    return PipelineRef(std::make_shared<VulkanPipeline>(
        std::move(device), PipelineKind::Compute, std::move(desc.layout),
        std::vector<ShaderRef>{std::move(desc.compute_shader)}, std::move(pipeline).value()));
}

VulkanPipeline::VulkanPipeline(std::shared_ptr<VulkanDeviceHandle> device, PipelineKind kind, PipelineLayoutRef layout,
                               std::vector<ShaderRef> shaders, vk::raii::Pipeline pipeline)
    : m_device(std::move(device)),
      m_kind(kind),
      m_layout(std::move(layout)),
      m_shaders(std::move(shaders)),
      m_pipeline(std::move(pipeline)) {
}

PipelineKind VulkanPipeline::getKind() const {
    return m_kind;
}

} // namespace rhi::vulkan
