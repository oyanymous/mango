#pragma once

#include <memory>

#include "IQueue.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalQueue final : public IQueue {
public:
    MetalQueue(std::shared_ptr<MetalDeviceContext> context, QueueType type, id<MTLCommandQueue> queue);

    ~MetalQueue() override;

    Result<void> submit(SubmitDesc desc) override;

    Result<void> present(PresentDesc desc) override;

    QueueType type() const { return m_type; }

    id<MTLCommandQueue> handle() const { return m_queue; }
private:
    QueueType m_type;
    id<MTLCommandQueue> m_queue = nil;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
