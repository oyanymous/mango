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
     * Takes ownership of a created VkShaderModule. Use create().
     *
     * @param reflection The shader's stage and entry point.
     */
    VulkanShader(std::shared_ptr<VulkanDeviceHandle> device, ShaderReflection reflection, VkShaderModule module);
    ~VulkanShader() override;

    VulkanShader(const VulkanShader&) = delete;
    VulkanShader& operator=(const VulkanShader&) = delete;

    ShaderReflection getReflection() const override;

    /// The VkShaderModule, for pipeline creation.
    VkShaderModule handle() const { return m_module; }
private:
    ShaderReflection m_reflection;
    VkShaderModule m_module = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
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
     * Takes ownership of a created VkPipelineLayout. Use create().
     *
     * @param desc The descriptor it was created with (retains the bind group layouts).
     */
    VulkanPipelineLayout(std::shared_ptr<VulkanDeviceHandle> device, PipelineLayoutDesc desc, VkPipelineLayout layout);
    ~VulkanPipelineLayout() override;

    VulkanPipelineLayout(const VulkanPipelineLayout&) = delete;
    VulkanPipelineLayout& operator=(const VulkanPipelineLayout&) = delete;

    PipelineLayoutDesc getDesc() const override;

    /// The VkPipelineLayout, for pipeline creation and descriptor binding.
    VkPipelineLayout handle() const { return m_layout; }
private:
    PipelineLayoutDesc m_desc;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
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
     * Takes ownership of a created VkPipeline. Use createGraphics() or createCompute().
     *
     * @param layout The layout it was compiled against; retained.
     * @param shaders The shaders it was compiled from; retained.
     */
    VulkanPipeline(std::shared_ptr<VulkanDeviceHandle> device, PipelineKind kind, PipelineLayoutRef layout,
                   std::vector<ShaderRef> shaders, VkPipeline pipeline);
    ~VulkanPipeline() override;

    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    PipelineKind getKind() const override;

    /// The VkPipeline, for vkCmdBindPipeline.
    VkPipeline handle() const { return m_pipeline; }
    /// The VkPipelineLayout the pipeline was compiled against, for vkCmdBindDescriptorSets.
    VkPipelineLayout layoutHandle() const { return static_cast<const VulkanPipelineLayout&>(*m_layout).handle(); }
private:
    PipelineKind m_kind;
    PipelineLayoutRef m_layout;
    std::vector<ShaderRef> m_shaders;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
