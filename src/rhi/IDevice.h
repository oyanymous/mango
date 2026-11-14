#pragma once

#include <string>

#include "IBindGroup.h"
#include "IBindGroupLayout.h"
#include "IBuffer.h"
#include "ICommandBuffer.h"
#include "IFence.h"
#include "IPipeline.h"
#include "IPipelineLayout.h"
#include "IQueue.h"
#include "IRenderPass.h"
#include "IRenderTarget.h"
#include "ISampler.h"
#include "ISemaphore.h"
#include "IShader.h"
#include "ISwapchain.h"
#include "ITexture.h"
#include "ITextureView.h"
#include "RHIFwd.h"
#include "RHITypes.h"
#include "Result.h"

namespace rhi {

// Optional features. Creating a device fails if a requested feature is unsupported
// (check IAdapter::getCapabilities() first).
struct DeviceFeatures {
    bool sampler_anisotropy = false;
    bool fill_mode_wireframe = false;
};

struct DeviceDesc {
    DeviceFeatures features;
    std::string label;
};

// The logical GPU: owns the queues and creates every other object.
class IDevice {
public:
    virtual ~IDevice() = default;

    // Falls back to the graphics queue when the adapter has no dedicated queue of that type.
    virtual QueueRef getQueue(QueueType type) = 0;

    virtual Result<BufferRef> createBuffer(BufferDesc desc) = 0;
    virtual Result<TextureRef> createTexture(TextureDesc desc) = 0;
    virtual Result<TextureViewRef> createTextureView(TextureViewDesc desc) = 0;
    virtual Result<SamplerRef> createSampler(SamplerDesc desc) = 0;
    virtual Result<ShaderRef> createShader(ShaderDesc desc) = 0;
    virtual Result<PipelineLayoutRef> createPipelineLayout(PipelineLayoutDesc desc) = 0;
    virtual Result<PipelineRef> createGraphicsPipeline(GraphicsPipelineDesc desc) = 0;
    virtual Result<PipelineRef> createComputePipeline(ComputePipelineDesc desc) = 0;
    virtual Result<RenderPassRef> createRenderPass(RenderPassDesc desc) = 0;
    virtual Result<RenderTargetRef> createRenderTarget(RenderTargetDesc desc) = 0;
    virtual Result<BindGroupLayoutRef> createBindGroupLayout(BindGroupLayoutDesc desc) = 0;
    virtual Result<BindGroupRef> createBindGroup(BindGroupDesc desc) = 0;
    virtual Result<CommandBufferRef> createCommandBuffer(CommandBufferDesc desc) = 0;
    virtual Result<SwapchainRef> createSwapchain(SwapchainDesc desc) = 0;
    virtual Result<FenceRef> createFence(FenceDesc desc) = 0;
    virtual Result<SemaphoreRef> createSemaphore(SemaphoreDesc desc) = 0;
};

} // namespace rhi
