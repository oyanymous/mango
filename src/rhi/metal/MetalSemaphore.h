#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include "ISemaphore.h"
#include "MetalCommon.h"
#include "Result.h"

namespace rhi::metal {

class MetalSemaphore final : public ISemaphore {
public:
    static Result<SemaphoreRef> create(std::shared_ptr<MetalDeviceContext> context, SemaphoreDesc desc);

    MetalSemaphore(std::shared_ptr<MetalDeviceContext> context, SemaphoreKind kind, id<MTLSharedEvent> event);

    SemaphoreKind getKind() const override;
    std::uint64_t getValue() const override;

    Result<void> signal(std::uint64_t value) override;

    std::uint64_t nextSignalValue(std::uint64_t requested);

    Result<std::uint64_t> nextWaitValue(std::uint64_t requested);

    void signalFromHost();

    id<MTLSharedEvent> event() const { return m_event; }
private:
    SemaphoreKind m_kind;
    id<MTLSharedEvent> m_event = nil;
    std::atomic<std::uint64_t> m_binary_signals{0};
    std::atomic<std::uint64_t> m_binary_waits{0};
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
