#pragma once

#include <memory>

#include "IBuffer.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalBuffer final : public IBuffer {
public:
    static Result<BufferRef> create(std::shared_ptr<MetalDeviceContext> context, BufferDesc desc);

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
