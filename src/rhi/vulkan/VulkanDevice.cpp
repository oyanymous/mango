#include "VulkanDevice.h"

#include <string>

#include "VulkanBindGroup.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanFence.h"
#include "VulkanPipeline.h"
#include "VulkanQueue.h"
#include "VulkanRenderPass.h"
#include "VulkanSemaphore.h"
#include "VulkanSwapchain.h"
#include "VulkanTexture.h"

namespace rhi::vulkan {

VulkanDeviceHandle::VulkanDeviceHandle(std::shared_ptr<VulkanInstanceHandle> instance,
                                       vk::raii::PhysicalDevice physical_device)
    : m_instance(std::move(instance)), m_physical_device(std::move(physical_device)) {
}

VulkanDeviceHandle::~VulkanDeviceHandle() {
    if (*m_device) {
        (void)m_device.waitIdle();
    }
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

Result<ShaderRef> VulkanDevice::createShader(ShaderDesc desc) {
    return VulkanShader::create(m_device, std::move(desc));
}

Result<PipelineLayoutRef> VulkanDevice::createPipelineLayout(PipelineLayoutDesc desc) {
    return VulkanPipelineLayout::create(m_device, std::move(desc));
}

Result<PipelineRef> VulkanDevice::createGraphicsPipeline(GraphicsPipelineDesc desc) {
    return VulkanPipeline::createGraphics(m_device, std::move(desc), m_desc.features.fill_mode_wireframe);
}

Result<PipelineRef> VulkanDevice::createComputePipeline(ComputePipelineDesc desc) {
    return VulkanPipeline::createCompute(m_device, std::move(desc));
}

Result<RenderPassRef> VulkanDevice::createRenderPass(RenderPassDesc desc) {
    return VulkanRenderPass::create(m_device, std::move(desc));
}

Result<RenderTargetRef> VulkanDevice::createRenderTarget(RenderTargetDesc desc) {
    return VulkanRenderTarget::create(m_device, std::move(desc));
}

Result<BindGroupLayoutRef> VulkanDevice::createBindGroupLayout(BindGroupLayoutDesc desc) {
    return VulkanBindGroupLayout::create(m_device, std::move(desc));
}

Result<BindGroupRef> VulkanDevice::createBindGroup(BindGroupDesc desc) {
    return VulkanBindGroup::create(m_device, std::move(desc));
}

Result<CommandBufferRef> VulkanDevice::createCommandBuffer(CommandBufferDesc desc) {
    const auto& queue = static_cast<const VulkanQueue&>(*getQueue(desc.queue));
    return VulkanCommandBuffer::create(m_device, std::move(desc), queue.familyIndex());
}

Result<SwapchainRef> VulkanDevice::createSwapchain(SwapchainDesc desc) {
    return VulkanSwapchain::create(m_device, std::move(desc));
}

Result<FenceRef> VulkanDevice::createFence(FenceDesc desc) {
    return VulkanFence::create(m_device, std::move(desc));
}

Result<SemaphoreRef> VulkanDevice::createSemaphore(SemaphoreDesc desc) {
    return VulkanSemaphore::create(m_device, std::move(desc));
}

} // namespace rhi::vulkan
