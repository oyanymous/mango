#pragma once

#include <memory>

#include "IBuffer.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"

namespace rhi::vulkan {

/// The Vulkan IBuffer. Created by VulkanDevice::createBuffer.
class VulkanBuffer final : public IBuffer {
public:
    /**
     * Creates the VkBuffer and its memory.
     *
     * @return The buffer, or InvalidArgument for a zero size or no usage.
     */
    static Result<BufferRef> create(std::shared_ptr<VulkanDeviceHandle> device, BufferDesc desc);

    /// Takes ownership of a created buffer and its memory. Use create().
    VulkanBuffer(std::shared_ptr<VulkanDeviceHandle> device, BufferDesc desc, VmaAllocationPtr allocation,
                 vk::raii::Buffer buffer);
    /// Unmaps if still mapped (VMA must not free mapped memory); the members then free the buffer and memory.
    ~VulkanBuffer() override;

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    BufferDesc getDesc() const override;
    /// Maps with vmaMapMemory and invalidates the range, so GPU writes are visible on non-coherent memory.
    MapResult map(MapDesc desc) override;
    /// Flushes the mapped range (for non-coherent memory) and unmaps.
    void unmap() override;

    /// The VkBuffer, for recording commands.
    VkBuffer handle() const { return static_cast<VkBuffer>(*m_buffer); }
private:
    VmaAllocator allocator() const { return m_device->m_allocator.get(); }

    // Destroyed in reverse order: buffer, then its memory, then (maybe) the device.
    std::shared_ptr<VulkanDeviceHandle> m_device;
    BufferDesc m_desc;
    MapDesc m_mapped_range; // valid while m_mapped
    bool m_mapped = false;
    VmaAllocationPtr m_allocation;
    vk::raii::Buffer m_buffer;
};

} // namespace rhi::vulkan
