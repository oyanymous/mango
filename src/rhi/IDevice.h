#pragma once

/**
 * @file
 * @brief rhi::IDevice: the logical GPU that creates every other object.
 */

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

/**
 * @brief Optional device features. Check Capabilities::features before requesting them.
 */
struct DeviceFeatures {
    bool sampler_anisotropy = false;  /**< SamplerDesc::max_anisotropy > 1. */
    bool fill_mode_wireframe = false; /**< RasterState::wireframe. */
};

/**
 * @brief Describes a device to create with IAdapter::createDevice.
 */
struct DeviceDesc {
    DeviceFeatures features; /**< Features to enable; creation fails if one is unsupported. */
    std::string label;       /**< Debug name. */
};

/**
 * @brief The logical GPU: owns the queues and creates every other object.
 *
 * @details
 * Every `create*` call takes a descriptor and returns the new object or an Error. Objects keep the device
 * alive, so it is safe to drop the DeviceRef before them.
 */
class IDevice {
public:
    virtual ~IDevice() = default;

    /**
     * @brief Returns the queue of a type. Falls back to the graphics queue when the adapter has no dedicated queue
     * of that type.
     *
     * @param type Queue type.
     */
    virtual QueueRef getQueue(QueueType type) = 0;

    /**
     * @brief Creates a buffer.
     *
     * @param desc Buffer descriptor.
     */
    virtual Result<BufferRef> createBuffer(BufferDesc desc) = 0;
    /**
     * @brief Creates a texture.
     *
     * @param desc Texture descriptor.
     */
    virtual Result<TextureRef> createTexture(TextureDesc desc) = 0;
    /**
     * @brief Creates a view of a texture.
     *
     * @param desc View descriptor, including the texture.
     */
    virtual Result<TextureViewRef> createTextureView(TextureViewDesc desc) = 0;
    /**
     * @brief Creates a sampler.
     *
     * @param desc Sampler descriptor.
     */
    virtual Result<SamplerRef> createSampler(SamplerDesc desc) = 0;
    /**
     * @brief Creates a shader module from SPIR-V.
     *
     * @param desc Shader descriptor.
     */
    virtual Result<ShaderRef> createShader(ShaderDesc desc) = 0;
    /**
     * @brief Creates a pipeline layout.
     *
     * @param desc Bind group layouts, in set order.
     */
    virtual Result<PipelineLayoutRef> createPipelineLayout(PipelineLayoutDesc desc) = 0;
    /**
     * @brief Creates a graphics pipeline. Can be slow (shader compilation); create pipelines up front.
     *
     * @param desc Shaders, vertex layout and fixed-function state.
     */
    virtual Result<PipelineRef> createGraphicsPipeline(GraphicsPipelineDesc desc) = 0;
    /**
     * @brief Creates a compute pipeline.
     *
     * @param desc Shader and layout.
     */
    virtual Result<PipelineRef> createComputePipeline(ComputePipelineDesc desc) = 0;
    /**
     * @brief Creates a render pass.
     *
     * @param desc Attachment formats and load/store ops.
     */
    virtual Result<RenderPassRef> createRenderPass(RenderPassDesc desc) = 0;
    /**
     * @brief Creates a render target.
     *
     * @param desc Pass and attachment views.
     */
    virtual Result<RenderTargetRef> createRenderTarget(RenderTargetDesc desc) = 0;
    /**
     * @brief Creates a bind group layout.
     *
     * @param desc Binding slots.
     */
    virtual Result<BindGroupLayoutRef> createBindGroupLayout(BindGroupLayoutDesc desc) = 0;
    /**
     * @brief Creates a bind group.
     *
     * @param desc Layout and resources.
     */
    virtual Result<BindGroupRef> createBindGroup(BindGroupDesc desc) = 0;
    /**
     * @brief Creates a command buffer.
     *
     * @param desc Target queue type.
     */
    virtual Result<CommandBufferRef> createCommandBuffer(CommandBufferDesc desc) = 0;
    /**
     * @brief Creates a swapchain on a surface.
     *
     * @param desc Surface, size, format and present mode.
     */
    virtual Result<SwapchainRef> createSwapchain(SwapchainDesc desc) = 0;
    /**
     * @brief Creates a fence.
     *
     * @param desc Initial state.
     */
    virtual Result<FenceRef> createFence(FenceDesc desc) = 0;
    /**
     * @brief Creates a semaphore.
     *
     * @param desc Kind and initial value.
     */
    virtual Result<SemaphoreRef> createSemaphore(SemaphoreDesc desc) = 0;
};

} // namespace rhi
