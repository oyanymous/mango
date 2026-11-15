#pragma once

#include <array>
#include <memory>

#include "IDevice.h"
#include "MetalCommon.h"

namespace rhi::metal {

/**
 * A Metal device: the shared MetalDeviceContext plus one MetalQueue per QueueType.
 *
 * Every object it creates holds the context, so objects may outlive the device. Metal objects are thread-safe, but
 * like the Vulkan backend this one is not: use a device and its objects from one thread at a time.
 */
class MetalDevice final : public IDevice {
public:
    /**
     * Creates the device's queues and wraps them.
     *
     * @param context Device context, shared with every object the device creates.
     * @param desc The descriptor it was created with (features already checked by the adapter).
     */
    static Result<DeviceRef> create(std::shared_ptr<MetalDeviceContext> context, DeviceDesc desc);

    /**
     * Wraps a context and its queues. Use create().
     *
     * @param queues One queue per QueueType, indexed by it.
     */
    MetalDevice(std::shared_ptr<MetalDeviceContext> context, DeviceDesc desc, std::array<QueueRef, 3> queues);

    /// Returns the queue of a type. Every type has its own `MTLCommandQueue`.
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
    std::array<QueueRef, 3> m_queues; // indexed by QueueType
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
