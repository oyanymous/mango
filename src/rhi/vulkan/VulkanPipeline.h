#pragma once

/**
 * @file
 * @brief rhi::vulkan::VulkanShader, VulkanPipelineLayout and VulkanPipeline.
 */

#include <memory>

#include "IPipeline.h"
#include "IPipelineLayout.h"
#include "IShader.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/**
 * @brief The Vulkan IShader: a VkShaderModule. Created by VulkanDevice::createShader.
 */
class VulkanShader final : public IShader {
public:
    /**
     * @brief Creates the VkShaderModule.
     *
     * @param device Owner of the VkDevice; kept alive by the shader.
     * @param desc One stage (Vertex, Fragment or Compute) and its SPIR-V.
     * @return The shader; InvalidArgument when the SPIR-V is empty or the stage is not a single stage.
     */
    static Result<ShaderRef> create(std::shared_ptr<VulkanDeviceHandle> device, ShaderDesc desc);

    /**
     * @brief Takes ownership of a created VkShaderModule. Use create().
     *
     * @param device Owner of the VkDevice.
     * @param reflection The shader's stage and entry point.
     * @param module The module.
     */
    VulkanShader(std::shared_ptr<VulkanDeviceHandle> device, ShaderReflection reflection, VkShaderModule module);
    /**
     * @brief Destroys the VkShaderModule.
     */
    ~VulkanShader() override;

    VulkanShader(const VulkanShader&) = delete;
    VulkanShader& operator=(const VulkanShader&) = delete;

    ShaderReflection getReflection() const override;

    /**
     * @brief The VkShaderModule, for pipeline creation.
     */
    VkShaderModule handle() const { return m_module; }
private:
    ShaderReflection m_reflection;
    VkShaderModule m_module = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

/**
 * @brief The Vulkan IPipelineLayout: a VkPipelineLayout. Created by VulkanDevice::createPipelineLayout.
 */
class VulkanPipelineLayout final : public IPipelineLayout {
public:
    /**
     * @brief Creates the VkPipelineLayout.
     *
     * @param device Owner of the VkDevice; kept alive by the layout.
     * @param desc Bind group layouts, in set order.
     * @return The layout; Unsupported when @p desc lists bind group layouts (they arrive in step 11).
     */
    static Result<PipelineLayoutRef> create(std::shared_ptr<VulkanDeviceHandle> device, PipelineLayoutDesc desc);

    /**
     * @brief Takes ownership of a created VkPipelineLayout. Use create().
     *
     * @param device Owner of the VkDevice.
     * @param desc The descriptor it was created with (retains the bind group layouts).
     * @param layout The layout.
     */
    VulkanPipelineLayout(std::shared_ptr<VulkanDeviceHandle> device, PipelineLayoutDesc desc, VkPipelineLayout layout);
    /**
     * @brief Destroys the VkPipelineLayout.
     */
    ~VulkanPipelineLayout() override;

    VulkanPipelineLayout(const VulkanPipelineLayout&) = delete;
    VulkanPipelineLayout& operator=(const VulkanPipelineLayout&) = delete;

    PipelineLayoutDesc getDesc() const override;

    /**
     * @brief The VkPipelineLayout, for pipeline creation and descriptor binding.
     */
    VkPipelineLayout handle() const { return m_layout; }
private:
    PipelineLayoutDesc m_desc;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

/**
 * @brief The Vulkan IPipeline: a VkPipeline. Retains its layout and shaders.
 *
 * @details
 * Graphics pipelines use a dynamic viewport and scissor, which VulkanCommandBuffer::beginRender sets.
 */
class VulkanPipeline final : public IPipeline {
public:
    /**
     * @brief Creates a graphics pipeline.
     *
     * @param device Owner of the VkDevice; kept alive by the pipeline.
     * @param desc Shaders, vertex layout, fixed-function state and the render pass it is compatible with.
     * @param wireframe_enabled Whether the device enabled DeviceFeatures::fill_mode_wireframe.
     * @return The pipeline; InvalidArgument when the layout, a shader or the render pass is missing, a shader
     *         has the wrong stage, or `blend` is neither empty nor one entry per colour attachment;
     *         Unsupported for a wireframe pipeline without the feature.
     */
    static Result<PipelineRef> createGraphics(std::shared_ptr<VulkanDeviceHandle> device, GraphicsPipelineDesc desc,
                                              bool wireframe_enabled);

    /**
     * @brief Takes ownership of a created VkPipeline. Use createGraphics().
     *
     * @param device Owner of the VkDevice.
     * @param kind Graphics or compute.
     * @param layout The layout it was compiled against; retained.
     * @param shaders The shaders it was compiled from; retained.
     * @param pipeline The pipeline.
     */
    VulkanPipeline(std::shared_ptr<VulkanDeviceHandle> device, PipelineKind kind, PipelineLayoutRef layout,
                   std::vector<ShaderRef> shaders, VkPipeline pipeline);
    /**
     * @brief Destroys the VkPipeline.
     */
    ~VulkanPipeline() override;

    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    PipelineKind getKind() const override;

    /**
     * @brief The VkPipeline, for vkCmdBindPipeline.
     */
    VkPipeline handle() const { return m_pipeline; }
private:
    PipelineKind m_kind;
    PipelineLayoutRef m_layout;
    std::vector<ShaderRef> m_shaders;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
