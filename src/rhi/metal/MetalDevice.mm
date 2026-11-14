/**
 * @file
 * @brief MetalDevice: queue creation and object factories.
 */

#include "MetalDevice.h"

#include <string>
#include <utility>

#include "MetalBuffer.h"
#include "MetalCommandBuffer.h"
#include "MetalFence.h"
#include "MetalPipeline.h"
#include "MetalQueue.h"
#include "MetalRenderPass.h"
#include "MetalSemaphore.h"
#include "MetalShader.h"
#include "MetalSwapchain.h"
#include "MetalTexture.h"

namespace rhi::metal {

namespace {

/**
 * @brief The error every factory of the backend not written yet returns.
 */
Error notImplemented(const char* what) {
    return Error{ErrorCode::Unsupported, std::string("MetalDevice::") + what + " is not implemented yet"};
}

/**
 * @brief Debug name of a queue, e.g. "smoke device graphics queue".
 */
NSString* queueLabel(const std::string& device_label, const char* type) {
    std::string label = device_label.empty() ? std::string(type) : device_label + " " + type;
    return toNSString(label + " queue");
}

} // namespace

Result<DeviceRef> MetalDevice::create(std::shared_ptr<MetalDeviceContext> context, DeviceDesc desc) {
    constexpr const char* kQueueNames[] = {"graphics", "compute", "transfer"};
    std::array<QueueRef, 3> queues;
    for (std::size_t i = 0; i < queues.size(); ++i) {
        id<MTLCommandQueue> queue = [context->device newCommandQueue];
        if (queue == nil) {
            return Error{ErrorCode::InitializationFailed, "MTLDevice newCommandQueue failed"};
        }
        queue.label = queueLabel(desc.label, kQueueNames[i]);
        queues[i] = std::make_shared<MetalQueue>(context, static_cast<QueueType>(i), queue);
    }
    return DeviceRef(std::make_shared<MetalDevice>(std::move(context), std::move(desc), std::move(queues)));
}

MetalDevice::MetalDevice(std::shared_ptr<MetalDeviceContext> context, DeviceDesc desc, std::array<QueueRef, 3> queues)
    : m_desc(std::move(desc)), m_queues(std::move(queues)), m_context(std::move(context)) {
}

QueueRef MetalDevice::getQueue(QueueType type) {
    return m_queues[static_cast<std::size_t>(type)];
}

Result<BufferRef> MetalDevice::createBuffer(BufferDesc desc) {
    return MetalBuffer::create(m_context, std::move(desc));
}

Result<TextureRef> MetalDevice::createTexture(TextureDesc desc) {
    return MetalTexture::create(m_context, std::move(desc));
}

Result<TextureViewRef> MetalDevice::createTextureView(TextureViewDesc desc) {
    return MetalTextureView::create(m_context, std::move(desc));
}

Result<SamplerRef> MetalDevice::createSampler(SamplerDesc desc) {
    return MetalSampler::create(m_context, std::move(desc), m_desc.features.sampler_anisotropy);
}

Result<ShaderRef> MetalDevice::createShader(ShaderDesc desc) {
    return MetalShader::create(m_context, std::move(desc));
}

Result<PipelineLayoutRef> MetalDevice::createPipelineLayout(PipelineLayoutDesc desc) {
    return MetalPipelineLayout::create(std::move(desc));
}

Result<PipelineRef> MetalDevice::createGraphicsPipeline(GraphicsPipelineDesc desc) {
    return MetalPipeline::createGraphics(m_context, std::move(desc), m_desc.features.fill_mode_wireframe);
}

Result<PipelineRef> MetalDevice::createComputePipeline(ComputePipelineDesc desc) {
    return MetalPipeline::createCompute(m_context, std::move(desc));
}

Result<RenderPassRef> MetalDevice::createRenderPass(RenderPassDesc desc) {
    return MetalRenderPass::create(std::move(desc));
}

Result<RenderTargetRef> MetalDevice::createRenderTarget(RenderTargetDesc desc) {
    return MetalRenderTarget::create(std::move(desc));
}

Result<BindGroupLayoutRef> MetalDevice::createBindGroupLayout(BindGroupLayoutDesc desc) {
    return MetalBindGroupLayout::create(std::move(desc));
}

Result<BindGroupRef> MetalDevice::createBindGroup(BindGroupDesc) {
    return notImplemented("createBindGroup");
}

Result<CommandBufferRef> MetalDevice::createCommandBuffer(CommandBufferDesc desc) {
    id<MTLCommandQueue> queue = static_cast<const MetalQueue&>(*getQueue(desc.queue)).handle();
    return CommandBufferRef(std::make_shared<MetalCommandBuffer>(m_context, std::move(desc), queue));
}

Result<SwapchainRef> MetalDevice::createSwapchain(SwapchainDesc desc) {
    return MetalSwapchain::create(m_context, std::move(desc));
}

Result<FenceRef> MetalDevice::createFence(FenceDesc desc) {
    return FenceRef(std::make_shared<MetalFence>(std::move(desc)));
}

Result<SemaphoreRef> MetalDevice::createSemaphore(SemaphoreDesc desc) {
    return MetalSemaphore::create(m_context, std::move(desc));
}

} // namespace rhi::metal
