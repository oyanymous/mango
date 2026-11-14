/// @file
/// VulkanDevice and VulkanDeviceHandle.

#include "VulkanDevice.h"

#include <string>

namespace rhi::vulkan {

namespace {

/// The error every not-yet-implemented create* method returns.
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

Result<BufferRef> VulkanDevice::createBuffer(BufferDesc) {
    return notImplemented("createBuffer", 6);
}

Result<TextureRef> VulkanDevice::createTexture(TextureDesc) {
    return notImplemented("createTexture", 7);
}

Result<TextureViewRef> VulkanDevice::createTextureView(TextureViewDesc) {
    return notImplemented("createTextureView", 7);
}

Result<SamplerRef> VulkanDevice::createSampler(SamplerDesc) {
    return notImplemented("createSampler", 7);
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

Result<CommandBufferRef> VulkanDevice::createCommandBuffer(CommandBufferDesc) {
    return notImplemented("createCommandBuffer", 6);
}

Result<SwapchainRef> VulkanDevice::createSwapchain(SwapchainDesc) {
    return notImplemented("createSwapchain", 9);
}

Result<FenceRef> VulkanDevice::createFence(FenceDesc) {
    return notImplemented("createFence", 5);
}

Result<SemaphoreRef> VulkanDevice::createSemaphore(SemaphoreDesc) {
    return notImplemented("createSemaphore", 5);
}

} // namespace rhi::vulkan
