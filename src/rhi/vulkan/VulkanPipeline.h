#pragma once

#include <memory>

#include "IPipeline.h"
#include "IPipelineLayout.h"
#include "IShader.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/// The Vulkan IShader: a VkShaderModule. Created by VulkanDevice::createShader.
class VulkanShader final : public IShader {
public:
    /**
     * Creates the VkShaderModule.
     *
     * @param desc One stage (Vertex, Fragment or Compute) and its SPIR-V.
     * @return The shader; InvalidArgument when the SPIR-V is empty or the stage is not a single stage.
     */
    static Result<ShaderRef> create(std::shared_ptr<VulkanDeviceHandle> device, ShaderDesc desc);

    /**
     * Takes ownership of a created shader module. Use create().
     *
     * @param reflection The shader's stage and entry point.
     */
    VulkanShader(std::shared_ptr<VulkanDeviceHandle> device, ShaderReflection reflection,
                 vk::raii::ShaderModule module);

    VulkanShader(const VulkanShader&) = delete;
    VulkanShader& operator=(const VulkanShader&) = delete;

    ShaderReflection getReflection() const override;

    vk::ShaderModule handle() const { return *m_module; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device; // first: destroyed after the module
    ShaderReflection m_reflection;
    vk::raii::ShaderModule m_module;
};

/// The Vulkan IPipelineLayout: a VkPipelineLayout. Created by VulkanDevice::createPipelineLayout.
class VulkanPipelineLayout final : public IPipelineLayout {
public:
    /**
     * Creates the VkPipelineLayout.
     *
     * @param desc Bind group layouts, in set order.
     * @return The layout; InvalidArgument when a bind group layout is null.
     */
    static Result<PipelineLayoutRef> create(std::shared_ptr<VulkanDeviceHandle> device, PipelineLayoutDesc desc);

    /**
     * Takes ownership of a created pipeline layout. Use create().
     *
     * @param desc The descriptor it was created with (retains the bind group layouts).
     */
    VulkanPipelineLayout(std::shared_ptr<VulkanDeviceHandle> device, PipelineLayoutDesc desc,
                         vk::raii::PipelineLayout layout);

    VulkanPipelineLayout(const VulkanPipelineLayout&) = delete;
    VulkanPipelineLayout& operator=(const VulkanPipelineLayout&) = delete;

    PipelineLayoutDesc getDesc() const override;

    vk::PipelineLayout handle() const { return *m_layout; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device; // first: destroyed after the layout
    PipelineLayoutDesc m_desc;
    vk::raii::PipelineLayout m_layout;
};

/**
 * The Vulkan IPipeline: a VkPipeline. Retains its layout and shaders.
 *
 * Graphics pipelines use a dynamic viewport and scissor, which VulkanCommandBuffer::beginRender sets.
 */
class VulkanPipeline final : public IPipeline {
public:
    /**
     * Creates a graphics pipeline.
     *
     * @param wireframe_enabled Whether the device enabled DeviceFeatures::fill_mode_wireframe.
     * @return The pipeline; InvalidArgument when the layout, a shader or the render pass is missing, a shader
     *         has the wrong stage, or `blend` is neither empty nor one entry per colour attachment;
     *         Unsupported for a wireframe pipeline without the feature.
     */
    static Result<PipelineRef> createGraphics(std::shared_ptr<VulkanDeviceHandle> device, GraphicsPipelineDesc desc,
                                              bool wireframe_enabled);
    /**
     * Creates a compute pipeline.
     *
     * @return The pipeline; InvalidArgument when the layout or shader is missing or the shader is not a
     *         ShaderStage::Compute shader.
     */
    static Result<PipelineRef> createCompute(std::shared_ptr<VulkanDeviceHandle> device, ComputePipelineDesc desc);

    /**
     * Takes ownership of a created pipeline. Use createGraphics() or createCompute().
     *
     * @param layout The layout it was compiled against; retained.
     * @param shaders The shaders it was compiled from; retained.
     */
    VulkanPipeline(std::shared_ptr<VulkanDeviceHandle> device, PipelineKind kind, PipelineLayoutRef layout,
                   std::vector<ShaderRef> shaders, vk::raii::Pipeline pipeline);

    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    PipelineKind getKind() const override;

    vk::Pipeline handle() const { return *m_pipeline; }
    /// The layout the pipeline was compiled against, for binding descriptor sets.
    vk::PipelineLayout layoutHandle() const { return static_cast<const VulkanPipelineLayout&>(*m_layout).handle(); }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device; // first: destroyed after the pipeline
    PipelineKind m_kind;
    PipelineLayoutRef m_layout;
    std::vector<ShaderRef> m_shaders;
    vk::raii::Pipeline m_pipeline;
};

} // namespace rhi::vulkan
