#pragma once

#include <array>
#include <memory>

#include <vk_mem_alloc.h>

#include "IDevice.h"
#include "VulkanCommon.h"
#include "VulkanInstance.h"

namespace rhi::vulkan {

/**
 * Owns the VkDevice and its VMA allocator.
 *
 * Shared by the device, its queues and every object created from it, so the VkDevice outlives all of them
 * without the device and its queues owning each other. Keeps the instance alive in turn.
 */
class VulkanDeviceHandle {
public:
    /**
     * Creates an empty handle; VulkanAdapter::createDevice fills in the members.
     *
     * @param instance Owner of the VkInstance the device belongs to.
     */
    explicit VulkanDeviceHandle(std::shared_ptr<VulkanInstanceHandle> instance);
    /// Waits for the GPU to go idle, then destroys the allocator and the device.
    ~VulkanDeviceHandle();

    VulkanDeviceHandle(const VulkanDeviceHandle&) = delete;
    VulkanDeviceHandle& operator=(const VulkanDeviceHandle&) = delete;

    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE; /**< The adapter's physical device. */
    VkDevice m_device = VK_NULL_HANDLE;                  /**< The device; destroyed with the handle. */
    VmaAllocator m_allocator = VK_NULL_HANDLE;           /**< Allocates all buffer and image memory. */
    std::uint32_t m_graphics_family = 0;                 /**< Queue family of the graphics (present) queue. */
    bool m_swapchain_enabled = false;                    /**< Whether VK_KHR_swapchain was enabled. */
private:
    std::shared_ptr<VulkanInstanceHandle> m_instance;
};

/**
 * The Vulkan IDevice. Created by VulkanAdapter::createDevice.
 *
 * The create* methods are filled in by later steps of docs/RHI_VULKAN_PLAN.md; until then they return
 * ErrorCode::Unsupported.
 */
class VulkanDevice final : public IDevice {
public:
    /**
     * Wraps an already created device.
     *
     * @param queues One queue per QueueType, indexed by its value; types without a dedicated family share
     *               the graphics queue object.
     */
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
    std::array<QueueRef, 3> m_queues; // indexed by QueueType
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
