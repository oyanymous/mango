#pragma once

#include <memory>

#include "IBuffer.h"
#include "MetalCommon.h"

namespace rhi::metal {

/**
 * A buffer in `MTLStorageModePrivate` (MemoryLocation::GpuOnly) or `MTLStorageModeShared` (CpuToGpu,
 * GpuToCpu).
 *
 * Shared memory is coherent between CPU and GPU, so map() returns the buffer's `contents` and unmap() only ends the
 * mapping; no flush is needed. Metal tracks hazards on the buffer, so barriers record no GPU work for it.
 */
class MetalBuffer final : public IBuffer {
public:
    /**
     * Creates a buffer.
     *
     * @return The buffer, InvalidArgument for a zero size or usage, or OutOfDeviceMemory if Metal cannot allocate it.
     */
    static Result<BufferRef> create(std::shared_ptr<MetalDeviceContext> context, BufferDesc desc);

    /// Wraps a buffer. Use create().
    MetalBuffer(std::shared_ptr<MetalDeviceContext> context, BufferDesc desc, id<MTLBuffer> buffer);

    BufferDesc getDesc() const override;
    MapResult map(MapDesc desc) override;
    void unmap() override;

    id<MTLBuffer> handle() const { return m_buffer; }
private:
    BufferDesc m_desc;
    id<MTLBuffer> m_buffer = nil;
    bool m_mapped = false;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
