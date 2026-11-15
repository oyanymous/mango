#pragma once

#include <array>
#include <memory>

#include "IDevice.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalDevice final : public IDevice {
public:
    static Result<DeviceRef> create(std::shared_ptr<MetalDeviceContext> context, DeviceDesc desc);

    MetalDevice(std::shared_ptr<MetalDeviceContext> context, DeviceDesc desc, std::array<QueueRef, 3> queues);

    QueueRef getQueue(QueueType type) override;

    Result<BufferRef> createBuffer(BufferDesc desc) override;
    Result<TextureRef> createTexture(TextureDesc desc) override;
    Result<TextureViewRef> createTextureView(TextureViewDesc desc) override;
    Result<SamplerRef> createSampler(SamplerDesc desc) override;
    Result<ShaderRef> createShader(ShaderDesc desc) override;
    Result<PipelineLayoutRef> createPipelineLayout(PipelineLayoutDesc desc) override;
    Result<PipelineRef> createGraphicsPipeline(GraphicsPipelineDesc desc) override;
    Result<PipelineRef> createComputePipeline(ComputePipelineDesc desc) override;
    Result<RenderPassRef> createRenderPass(RenderPassDesc desc) override;
    Result<RenderTargetRef> createRenderTarget(RenderTargetDesc desc) override;
    Result<BindGroupLayoutRef> createBindGroupLayout(BindGroupLayoutDesc desc) override;
    Result<BindGroupRef> createBindGroup(BindGroupDesc desc) override;
    Result<CommandBufferRef> createCommandBuffer(CommandBufferDesc desc) override;
    Result<SwapchainRef> createSwapchain(SwapchainDesc desc) override;
    Result<FenceRef> createFence(FenceDesc desc) override;
    Result<SemaphoreRef> createSemaphore(SemaphoreDesc desc) override;
private:
    DeviceDesc m_desc;
    std::array<QueueRef, 3> m_queues;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
