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

VkPrimitiveTopology toVkTopology(PrimitiveTopology topology) {
    switch (topology) {
    case PrimitiveTopology::PointList:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case PrimitiveTopology::LineList:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case PrimitiveTopology::LineStrip:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case PrimitiveTopology::TriangleList:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case PrimitiveTopology::TriangleStrip:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

VkCullModeFlags toVkCullMode(CullMode mode) {
    switch (mode) {
    case CullMode::None:
        return VK_CULL_MODE_NONE;
    case CullMode::Front:
        return VK_CULL_MODE_FRONT_BIT;
    case CullMode::Back:
        return VK_CULL_MODE_BACK_BIT;
    }
    return VK_CULL_MODE_NONE;
}

/**
 * @brief Maps a BlendFactor; the enumerators are declared in Vulkan's order.
 */
VkBlendFactor toVkBlendFactor(BlendFactor factor) {
    static_assert(static_cast<int>(BlendFactor::OneMinusDstAlpha) == VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA);
    return static_cast<VkBlendFactor>(factor);
}

/**
 * @brief Maps a BlendOp; the enumerators are declared in Vulkan's order.
 */
VkBlendOp toVkBlendOp(BlendOp op) {
    static_assert(static_cast<int>(BlendOp::Max) == VK_BLEND_OP_MAX);
    return static_cast<VkBlendOp>(op);
}

VkPipelineColorBlendAttachmentState toVkBlend(const BlendState& blend) {
    VkPipelineColorBlendAttachmentState state{};
    state.blendEnable = blend.enabled ? VK_TRUE : VK_FALSE;
    state.srcColorBlendFactor = toVkBlendFactor(blend.src_color);
    state.dstColorBlendFactor = toVkBlendFactor(blend.dst_color);
    state.colorBlendOp = toVkBlendOp(blend.color_op);
    state.srcAlphaBlendFactor = toVkBlendFactor(blend.src_alpha);
    state.dstAlphaBlendFactor = toVkBlendFactor(blend.dst_alpha);
    state.alphaBlendOp = toVkBlendOp(blend.alpha_op);
    // ColorWriteMask uses Vulkan's R, G, B, A bit positions.
    state.colorWriteMask = static_cast<VkColorComponentFlags>(blend.write_mask);
    return state;
}

/**
 * @brief The shader stage create info for @p shader, which must be a VulkanShader.
 */
VkPipelineShaderStageCreateInfo toVkStage(const ShaderRef& shader, VkShaderStageFlagBits stage,
                                          const std::string& entry_point) {
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = stage;
    info.module = static_cast<const VulkanShader&>(*shader).handle();
    info.pName = entry_point.c_str();
    return info;
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

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = desc.spirv.size() * sizeof(std::uint32_t);
    info.pCode = desc.spirv.data();
    VkShaderModule module = VK_NULL_HANDLE;
    if (VkResult result = vkCreateShaderModule(device->m_device, &info, nullptr, &module); result != VK_SUCCESS) {
        return toError(result, "vkCreateShaderModule");
    }
    return ShaderRef(std::make_shared<VulkanShader>(
        std::move(device), ShaderReflection{.stage = desc.stage, .entry_point = std::move(desc.entry_point)}, module));
}

VulkanShader::VulkanShader(std::shared_ptr<VulkanDeviceHandle> device, ShaderReflection reflection,
                           VkShaderModule module)
    : m_reflection(std::move(reflection)), m_module(module), m_device(std::move(device)) {
}

VulkanShader::~VulkanShader() {
    vkDestroyShaderModule(m_device->m_device, m_module, nullptr);
}

ShaderReflection VulkanShader::getReflection() const {
    return m_reflection;
}

// --- VulkanPipelineLayout ---

Result<PipelineLayoutRef> VulkanPipelineLayout::create(std::shared_ptr<VulkanDeviceHandle> device,
                                                       PipelineLayoutDesc desc) {
    std::vector<VkDescriptorSetLayout> set_layouts;
    for (const BindGroupLayoutRef& layout : desc.bind_group_layouts) {
        if (!layout) {
            return Error{ErrorCode::InvalidArgument, "createPipelineLayout: a bind group layout is null"};
        }
        set_layouts.push_back(static_cast<const VulkanBindGroupLayout&>(*layout).handle());
    }

    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = static_cast<std::uint32_t>(set_layouts.size());
    info.pSetLayouts = set_layouts.data();
    VkPipelineLayout layout = VK_NULL_HANDLE;
    if (VkResult result = vkCreatePipelineLayout(device->m_device, &info, nullptr, &layout); result != VK_SUCCESS) {
        return toError(result, "vkCreatePipelineLayout");
    }
    return PipelineLayoutRef(std::make_shared<VulkanPipelineLayout>(std::move(device), std::move(desc), layout));
}

VulkanPipelineLayout::VulkanPipelineLayout(std::shared_ptr<VulkanDeviceHandle> device, PipelineLayoutDesc desc,
                                           VkPipelineLayout layout)
    : m_desc(std::move(desc)), m_layout(layout), m_device(std::move(device)) {
}

VulkanPipelineLayout::~VulkanPipelineLayout() {
    vkDestroyPipelineLayout(m_device->m_device, m_layout, nullptr);
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

    const std::array<VkPipelineShaderStageCreateInfo, 2> stages = {
        toVkStage(desc.vertex_shader, VK_SHADER_STAGE_VERTEX_BIT, vertex.entry_point),
        toVkStage(desc.fragment_shader, VK_SHADER_STAGE_FRAGMENT_BIT, fragment.entry_point),
    };

    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    for (std::uint32_t slot = 0; slot < desc.vertex_buffers.size(); ++slot) {
        const VertexBufferLayout& layout = desc.vertex_buffers[slot];
        bindings.push_back({slot, layout.stride,
                            layout.step_mode == VertexStepMode::Instance ? VK_VERTEX_INPUT_RATE_INSTANCE
                                                                         : VK_VERTEX_INPUT_RATE_VERTEX});
        for (const VertexAttribute& attribute : layout.attributes) {
            attributes.push_back({attribute.location, slot, toVkFormat(attribute.format), attribute.offset});
        }
    }
    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = static_cast<std::uint32_t>(bindings.size());
    vertex_input.pVertexBindingDescriptions = bindings.data();
    vertex_input.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
    vertex_input.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = toVkTopology(desc.topology);

    // Viewport and scissor are dynamic; only their counts are fixed here.
    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = desc.raster.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    raster.cullMode = toVkCullMode(desc.raster.cull_mode);
    raster.frontFace =
        desc.raster.front_face == FrontFace::Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;

    // Every attachment of a pass has the same sample count.
    const std::uint32_t samples = !pass.color_attachments.empty() ? pass.color_attachments[0].sample_count
                                                                  : pass.depth_stencil_attachment->sample_count;
    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = static_cast<VkSampleCountFlagBits>(samples);

    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = desc.depth_stencil.depth_test ? VK_TRUE : VK_FALSE;
    depth.depthWriteEnable = desc.depth_stencil.depth_write ? VK_TRUE : VK_FALSE;
    depth.depthCompareOp = toVkCompareOp(desc.depth_stencil.depth_compare);

    std::vector<VkPipelineColorBlendAttachmentState> blends;
    for (std::size_t i = 0; i < pass.color_attachments.size(); ++i) {
        blends.push_back(toVkBlend(desc.blend.empty() ? BlendState{} : desc.blend[i]));
    }
    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = static_cast<std::uint32_t>(blends.size());
    blend.pAttachments = blends.data();

    const std::array<VkDynamicState, 2> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
    dynamic.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = static_cast<std::uint32_t>(stages.size());
    info.pStages = stages.data();
    info.pVertexInputState = &vertex_input;
    info.pInputAssemblyState = &input_assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = pass.depth_stencil_attachment ? &depth : nullptr;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic;
    info.layout = static_cast<const VulkanPipelineLayout&>(*desc.layout).handle();
    info.renderPass = static_cast<const VulkanRenderPass&>(*desc.render_pass).handle();
    info.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (VkResult result = vkCreateGraphicsPipelines(device->m_device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline);
        result != VK_SUCCESS) {
        return toError(result, "vkCreateGraphicsPipelines");
    }
    return PipelineRef(std::make_shared<VulkanPipeline>(
        std::move(device), PipelineKind::Graphics, std::move(desc.layout),
        std::vector<ShaderRef>{std::move(desc.vertex_shader), std::move(desc.fragment_shader)}, pipeline));
}

VulkanPipeline::VulkanPipeline(std::shared_ptr<VulkanDeviceHandle> device, PipelineKind kind, PipelineLayoutRef layout,
                               std::vector<ShaderRef> shaders, VkPipeline pipeline)
    : m_kind(kind),
      m_layout(std::move(layout)),
      m_shaders(std::move(shaders)),
      m_pipeline(pipeline),
      m_device(std::move(device)) {
}

VulkanPipeline::~VulkanPipeline() {
    vkDestroyPipeline(m_device->m_device, m_pipeline, nullptr);
}

PipelineKind VulkanPipeline::getKind() const {
    return m_kind;
}

} // namespace rhi::vulkan
