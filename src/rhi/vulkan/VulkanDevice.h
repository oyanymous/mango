#pragma once

#include <array>
#include <memory>

#include <vk_mem_alloc.h>

#include "IDevice.h"
#include "VulkanCommon.h"
#include "VulkanInstance.h"

namespace rhi::vulkan {

struct VmaAllocatorDeleter {
    void operator()(VmaAllocator allocator) const { vmaDestroyAllocator(allocator); }
};

using VmaAllocatorPtr = std::unique_ptr<VmaAllocator_T, VmaAllocatorDeleter>;

struct VmaAllocationDeleter {
    VmaAllocator allocator = VK_NULL_HANDLE;
    void operator()(VmaAllocation allocation) const { vmaFreeMemory(allocator, allocation); }
};

using VmaAllocationPtr = std::unique_ptr<VmaAllocation_T, VmaAllocationDeleter>;

class VulkanDeviceHandle {
public:
    VulkanDeviceHandle(std::shared_ptr<VulkanInstanceHandle> instance, vk::raii::PhysicalDevice physical_device);

    ~VulkanDeviceHandle();

    VulkanDeviceHandle(const VulkanDeviceHandle&) = delete;
    VulkanDeviceHandle& operator=(const VulkanDeviceHandle&) = delete;
private:
    std::shared_ptr<VulkanInstanceHandle> m_instance;
public:
    vk::raii::PhysicalDevice m_physical_device;
    vk::raii::Device m_device = nullptr;
    VmaAllocatorPtr m_allocator;
    std::uint32_t m_graphics_family = 0;
    bool m_swapchain_enabled = false;
};

class VulkanDevice final : public IDevice {
public:
    VulkanDevice(DeviceDesc desc, std::shared_ptr<VulkanDeviceHandle> handle, std::array<QueueRef, 3> queues);

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
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
