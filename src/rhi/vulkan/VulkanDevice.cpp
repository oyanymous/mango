/**
 * @file
 * @brief VulkanDevice and VulkanDeviceHandle.
 */

#include "VulkanDevice.h"

#include <string>

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanFence.h"
#include "VulkanQueue.h"
#include "VulkanSemaphore.h"
#include "VulkanTexture.h"

namespace rhi::vulkan {

namespace {

/**
 * @brief The error every not-yet-implemented create* method returns.
 */
Error notImplemented(const char* function, int step) {
    return Error{ErrorCode::Unsupported,
                 std::string("VulkanDevice::") + function + " is implemented in step " + std::to_string(step)};
}

} // namespace

VulkanDeviceHandle::VulkanDeviceHandle(std::shared_ptr<VulkanInstanceHandle> instance)
    : m_instance(std::move(instance)) {
}

VulkanDeviceHandle::~VulkanDeviceHandle() {
    if (m_device == VK_NULL_HANDLE) {
        return;
    }
    // Nothing may still be running on the GPU when the device goes away.
    vkDeviceWaitIdle(m_device);
    if (m_allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(m_allocator);
    }
    vkDestroyDevice(m_device, nullptr);
}

VulkanDevice::VulkanDevice(DeviceDesc desc, std::shared_ptr<VulkanDeviceHandle> handle, std::array<QueueRef, 3> queues)
    : m_desc(std::move(desc)), m_queues(std::move(queues)), m_device(std::move(handle)) {
}

QueueRef VulkanDevice::getQueue(QueueType type) {
    return m_queues[static_cast<std::size_t>(type)];
}

Result<BufferRef> VulkanDevice::createBuffer(BufferDesc desc) {
    return VulkanBuffer::create(m_device, std::move(desc));
}

Result<TextureRef> VulkanDevice::createTexture(TextureDesc desc) {
    return VulkanTexture::create(m_device, std::move(desc));
}

Result<TextureViewRef> VulkanDevice::createTextureView(TextureViewDesc desc) {
    return VulkanTextureView::create(m_device, std::move(desc));
}

Result<SamplerRef> VulkanDevice::createSampler(SamplerDesc desc) {
    return VulkanSampler::create(m_device, std::move(desc), m_desc.features.sampler_anisotropy);
}

Result<ShaderRef> VulkanDevice::createShader(ShaderDesc) {
    return notImplemented("createShader", 10);
}

Result<PipelineLayoutRef> VulkanDevice::createPipelineLayout(PipelineLayoutDesc) {
    return notImplemented("createPipelineLayout", 10);
}

Result<PipelineRef> VulkanDevice::createGraphicsPipeline(GraphicsPipelineDesc) {
    return notImplemented("createGraphicsPipeline", 10);
}

Result<PipelineRef> VulkanDevice::createComputePipeline(ComputePipelineDesc) {
    return notImplemented("createComputePipeline", 12);
}

Result<RenderPassRef> VulkanDevice::createRenderPass(RenderPassDesc) {
    return notImplemented("createRenderPass", 8);
}

Result<RenderTargetRef> VulkanDevice::createRenderTarget(RenderTargetDesc) {
    return notImplemented("createRenderTarget", 8);
}

Result<BindGroupLayoutRef> VulkanDevice::createBindGroupLayout(BindGroupLayoutDesc) {
    return notImplemented("createBindGroupLayout", 11);
}

Result<BindGroupRef> VulkanDevice::createBindGroup(BindGroupDesc) {
    return notImplemented("createBindGroup", 11);
}

Result<CommandBufferRef> VulkanDevice::createCommandBuffer(CommandBufferDesc desc) {
    const auto& queue = static_cast<const VulkanQueue&>(*getQueue(desc.queue));
    return VulkanCommandBuffer::create(m_device, std::move(desc), queue.familyIndex());
}

Result<SwapchainRef> VulkanDevice::createSwapchain(SwapchainDesc) {
    return notImplemented("createSwapchain", 9);
}

Result<FenceRef> VulkanDevice::createFence(FenceDesc desc) {
    return VulkanFence::create(m_device, std::move(desc));
}

Result<SemaphoreRef> VulkanDevice::createSemaphore(SemaphoreDesc desc) {
    return VulkanSemaphore::create(m_device, std::move(desc));
}

} // namespace rhi::vulkan
