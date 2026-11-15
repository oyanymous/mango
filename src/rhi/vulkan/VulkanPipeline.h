#pragma once

#include <memory>

#include "IPipeline.h"
#include "IPipelineLayout.h"
#include "IShader.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

class VulkanShader final : public IShader {
public:
    static Result<ShaderRef> create(std::shared_ptr<VulkanDeviceHandle> device, ShaderDesc desc);

    VulkanShader(std::shared_ptr<VulkanDeviceHandle> device, ShaderReflection reflection,
                 vk::raii::ShaderModule module);

    VulkanShader(const VulkanShader&) = delete;
    VulkanShader& operator=(const VulkanShader&) = delete;

    ShaderReflection getReflection() const override;

    vk::ShaderModule handle() const { return *m_module; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    ShaderReflection m_reflection;
    vk::raii::ShaderModule m_module;
};

class VulkanPipelineLayout final : public IPipelineLayout {
public:
    static Result<PipelineLayoutRef> create(std::shared_ptr<VulkanDeviceHandle> device, PipelineLayoutDesc desc);

    VulkanPipelineLayout(std::shared_ptr<VulkanDeviceHandle> device, PipelineLayoutDesc desc,
                         vk::raii::PipelineLayout layout);

    VulkanPipelineLayout(const VulkanPipelineLayout&) = delete;
    VulkanPipelineLayout& operator=(const VulkanPipelineLayout&) = delete;

    PipelineLayoutDesc getDesc() const override;

    vk::PipelineLayout handle() const { return *m_layout; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    PipelineLayoutDesc m_desc;
    vk::raii::PipelineLayout m_layout;
};

class VulkanPipeline final : public IPipeline {
public:
    static Result<PipelineRef> createGraphics(std::shared_ptr<VulkanDeviceHandle> device, GraphicsPipelineDesc desc,
                                              bool wireframe_enabled);

    static Result<PipelineRef> createCompute(std::shared_ptr<VulkanDeviceHandle> device, ComputePipelineDesc desc);

    VulkanPipeline(std::shared_ptr<VulkanDeviceHandle> device, PipelineKind kind, PipelineLayoutRef layout,
                   std::vector<ShaderRef> shaders, vk::raii::Pipeline pipeline);

    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    PipelineKind getKind() const override;

    vk::Pipeline handle() const { return *m_pipeline; }

    vk::PipelineLayout layoutHandle() const { return static_cast<const VulkanPipelineLayout&>(*m_layout).handle(); }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    PipelineKind m_kind;
    PipelineLayoutRef m_layout;
    std::vector<ShaderRef> m_shaders;
    vk::raii::Pipeline m_pipeline;
};

} // namespace rhi::vulkan
