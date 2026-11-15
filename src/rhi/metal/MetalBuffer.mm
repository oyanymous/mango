#include "MetalBuffer.h"

#include <utility>

namespace rhi::metal {

Result<BufferRef> MetalBuffer::create(std::shared_ptr<MetalDeviceContext> context, BufferDesc desc) {
    if (desc.size == 0 || desc.usage == BufferUsage::None) {
        return Error{ErrorCode::InvalidArgument, "createBuffer: size and usage must be non-zero"};
    }
    if (desc.size > context->device.maxBufferLength) {
        return Error{ErrorCode::OutOfDeviceMemory, "createBuffer: size exceeds the device's maxBufferLength"};
    }

    const MTLResourceOptions options =
        desc.memory == MemoryLocation::GpuOnly ? MTLResourceStorageModePrivate : MTLResourceStorageModeShared;
    id<MTLBuffer> buffer = [context->device newBufferWithLength:desc.size options:options];
    if (buffer == nil) {
        return Error{ErrorCode::OutOfDeviceMemory, "MTLDevice newBufferWithLength failed"};
    }
    buffer.label = toNSString(desc.label);
    return BufferRef(std::make_shared<MetalBuffer>(std::move(context), std::move(desc), buffer));
}

MetalBuffer::MetalBuffer(std::shared_ptr<MetalDeviceContext> context, BufferDesc desc, id<MTLBuffer> buffer)
    : m_desc(std::move(desc)), m_buffer(buffer), m_context(std::move(context)) {
}

BufferDesc MetalBuffer::getDesc() const {
    return m_desc;
}

MapResult MetalBuffer::map(MapDesc desc) {
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
    m_mapped = true;
    return std::span<std::byte>(static_cast<std::byte*>(m_buffer.contents) + desc.offset, desc.size);
}

void MetalBuffer::unmap() {
    m_mapped = false;
}

} // namespace rhi::metal
