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

/// Optional device features. Check Capabilities::features before requesting them.
struct DeviceFeatures {
    bool sampler_anisotropy = false;  /**< SamplerDesc::max_anisotropy > 1. */
    bool fill_mode_wireframe = false; /**< RasterState::wireframe. */
};

/// Describes a device to create with IAdapter::createDevice.
struct DeviceDesc {
    DeviceFeatures features; /**< Features to enable; creation fails if one is unsupported. */
    std::string label;
};

/**
 * The logical GPU: owns the queues and creates every other object.
 *
 * Every `create*` call takes a descriptor and returns the new object or an Error. Objects keep the device
 * alive, so it is safe to drop the DeviceRef before them.
 */
class IDevice {
public:
    virtual ~IDevice() = default;

    /// Returns the queue of a type. Falls back to the graphics queue when the adapter has no dedicated queue of that
    /// type.
    virtual QueueRef getQueue(QueueType type) = 0;

    virtual Result<BufferRef> createBuffer(BufferDesc desc) = 0;
    virtual Result<TextureRef> createTexture(TextureDesc desc) = 0;
    /**
     * Creates a view of a texture.
     *
     * @param desc View descriptor, including the texture.
     */
    virtual Result<TextureViewRef> createTextureView(TextureViewDesc desc) = 0;
    virtual Result<SamplerRef> createSampler(SamplerDesc desc) = 0;
    /// Creates a shader module from SPIR-V.
    virtual Result<ShaderRef> createShader(ShaderDesc desc) = 0;
    /**
     * Creates a pipeline layout.
     *
     * @param desc Bind group layouts, in set order.
     */
    virtual Result<PipelineLayoutRef> createPipelineLayout(PipelineLayoutDesc desc) = 0;
    /// Creates a graphics pipeline. Can be slow (shader compilation); create pipelines up front.
    virtual Result<PipelineRef> createGraphicsPipeline(GraphicsPipelineDesc desc) = 0;
    virtual Result<PipelineRef> createComputePipeline(ComputePipelineDesc desc) = 0;
    virtual Result<RenderPassRef> createRenderPass(RenderPassDesc desc) = 0;
    virtual Result<RenderTargetRef> createRenderTarget(RenderTargetDesc desc) = 0;
    virtual Result<BindGroupLayoutRef> createBindGroupLayout(BindGroupLayoutDesc desc) = 0;
    virtual Result<BindGroupRef> createBindGroup(BindGroupDesc desc) = 0;
    virtual Result<CommandBufferRef> createCommandBuffer(CommandBufferDesc desc) = 0;
    /// Creates a swapchain on a surface.
    virtual Result<SwapchainRef> createSwapchain(SwapchainDesc desc) = 0;
    virtual Result<FenceRef> createFence(FenceDesc desc) = 0;
    virtual Result<SemaphoreRef> createSemaphore(SemaphoreDesc desc) = 0;
};

} // namespace rhi
