#pragma once

#include <memory>

#include "IBuffer.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"

namespace rhi::vulkan {

class VulkanBuffer final : public IBuffer {
public:
    static Result<BufferRef> create(std::shared_ptr<VulkanDeviceHandle> device, BufferDesc desc);

    VulkanBuffer(std::shared_ptr<VulkanDeviceHandle> device, BufferDesc desc, VmaAllocationPtr allocation,
                 vk::raii::Buffer buffer);

    ~VulkanBuffer() override;

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    BufferDesc getDesc() const override;

    MapResult map(MapDesc desc) override;

    void unmap() override;

    vk::Buffer handle() const { return *m_buffer; }
private:
    VmaAllocator allocator() const { return m_device->m_allocator.get(); }

    std::shared_ptr<VulkanDeviceHandle> m_device;
    BufferDesc m_desc;
    MapDesc m_mapped_range;
    bool m_mapped = false;
    VmaAllocationPtr m_allocation;
    vk::raii::Buffer m_buffer;
};

} // namespace rhi::vulkan
