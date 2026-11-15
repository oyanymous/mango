/**
 * @file
 * @brief VulkanBuffer.
 */

#include "VulkanBuffer.h"

#include "VulkanDevice.h"

namespace rhi::vulkan {

namespace {

/// Maps BufferUsage bits to VkBufferUsageFlags.
VkBufferUsageFlags toVkBufferUsage(BufferUsage usage) {
    VkBufferUsageFlags flags = 0;
    if (hasFlag(usage, BufferUsage::CopySrc)) {
        flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (hasFlag(usage, BufferUsage::CopyDst)) {
        flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (hasFlag(usage, BufferUsage::Vertex)) {
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (hasFlag(usage, BufferUsage::Index)) {
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (hasFlag(usage, BufferUsage::Uniform)) {
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (hasFlag(usage, BufferUsage::Storage)) {
        flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (hasFlag(usage, BufferUsage::Indirect)) {
        flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }
    return flags;
}

} // namespace

Result<BufferRef> VulkanBuffer::create(std::shared_ptr<VulkanDeviceHandle> device, BufferDesc desc) {
    if (desc.size == 0 || desc.usage == BufferUsage::None) {
        return Error{ErrorCode::InvalidArgument, "createBuffer: size and usage must be non-zero"};
    }

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = desc.size;
    buffer_info.usage = toVkBufferUsage(desc.usage);
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // VMA_MEMORY_USAGE_AUTO picks the memory type from the usage and these host-access hints.
    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    if (desc.memory == MemoryLocation::CpuToGpu) {
        allocation_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    } else if (desc.memory == MemoryLocation::GpuToCpu) {
        allocation_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    }

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    if (VkResult result =
            vmaCreateBuffer(device->m_allocator, &buffer_info, &allocation_info, &buffer, &allocation, nullptr);
        result != VK_SUCCESS) {
        return toError(result, "vmaCreateBuffer");
    }
    return BufferRef(std::make_shared<VulkanBuffer>(std::move(device), std::move(desc), buffer, allocation));
}

VulkanBuffer::VulkanBuffer(std::shared_ptr<VulkanDeviceHandle> device, BufferDesc desc, VkBuffer buffer,
                           VmaAllocation allocation)
    : m_desc(std::move(desc)), m_buffer(buffer), m_allocation(allocation), m_device(std::move(device)) {
}

VulkanBuffer::~VulkanBuffer() {
    if (m_mapped) {
        vmaUnmapMemory(m_device->m_allocator, m_allocation);
    }
    vmaDestroyBuffer(m_device->m_allocator, m_buffer, m_allocation);
}

BufferDesc VulkanBuffer::getDesc() const {
    return m_desc;
}

MapResult VulkanBuffer::map(MapDesc desc) {
    if (m_desc.memory == MemoryLocation::GpuOnly) {
        return Error{ErrorCode::InvalidArgument, "IBuffer::map: GpuOnly buffers cannot be mapped"};
    }
    if (m_mapped) {
        return Error{ErrorCode::InvalidArgument, "IBuffer::map: the buffer is already mapped"};
    }
    if (desc.offset > m_desc.size) {
        return Error{ErrorCode::InvalidArgument, "IBuffer::map: offset is past the end of the buffer"};
    }
    if (desc.size == kWholeSize) {
        desc.size = m_desc.size - desc.offset;
    }
    if (desc.size > m_desc.size - desc.offset) {
        return Error{ErrorCode::InvalidArgument, "IBuffer::map: range is past the end of the buffer"};
    }

    void* data = nullptr;
    if (VkResult result = vmaMapMemory(m_device->m_allocator, m_allocation, &data); result != VK_SUCCESS) {
        return toError(result, "vmaMapMemory");
    }
    // A no-op on coherent memory; needed to see GPU writes on non-coherent memory.
    if (VkResult result = vmaInvalidateAllocation(m_device->m_allocator, m_allocation, desc.offset, desc.size);
        result != VK_SUCCESS) {
        vmaUnmapMemory(m_device->m_allocator, m_allocation);
        return toError(result, "vmaInvalidateAllocation");
    }
    m_mapped = true;
    m_mapped_range = desc;
    return std::span<std::byte>(static_cast<std::byte*>(data) + desc.offset, desc.size);
}

void VulkanBuffer::unmap() {
    if (!m_mapped) {
        return;
    }
    vmaFlushAllocation(m_device->m_allocator, m_allocation, m_mapped_range.offset, m_mapped_range.size);
    vmaUnmapMemory(m_device->m_allocator, m_allocation);
    m_mapped = false;
}

} // namespace rhi::vulkan
