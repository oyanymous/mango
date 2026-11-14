#pragma once

/// @file
/// rhi::vulkan::VulkanBuffer: a VkBuffer with VMA-allocated memory.

#include <memory>

#include <vk_mem_alloc.h>

#include "IBuffer.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/// The Vulkan IBuffer. Created by VulkanDevice::createBuffer.
class VulkanBuffer final : public IBuffer {
public:
    /// Creates the VkBuffer and its memory.
    /// @param device Owner of the VkDevice and allocator; kept alive by the buffer.
    /// @param desc Size, usage and memory location.
    /// @return The buffer, or InvalidArgument for a zero size or no usage.
    static Result<BufferRef> create(std::shared_ptr<VulkanDeviceHandle> device, BufferDesc desc);

    /// Takes ownership of a created buffer and allocation. Use create().
    /// @param device Owner of the VkDevice.
    /// @param desc The descriptor it was created with.
    /// @param buffer The buffer.
    /// @param allocation Its memory.
    VulkanBuffer(std::shared_ptr<VulkanDeviceHandle> device, BufferDesc desc, VkBuffer buffer,
                 VmaAllocation allocation);
    /// Unmaps if needed, then destroys the buffer and frees its memory.
    ~VulkanBuffer() override;

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    BufferDesc getDesc() const override;
    /// Maps with vmaMapMemory and invalidates the range, so GPU writes are visible on non-coherent memory.
    MapResult map(MapDesc desc) override;
    /// Flushes the mapped range (for non-coherent memory) and unmaps.
    void unmap() override;

    /// The VkBuffer, for recording commands.
    VkBuffer handle() const { return m_buffer; }
private:
    BufferDesc m_desc;
    MapDesc m_mapped_range; // valid while m_mapped
    bool m_mapped = false;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
