#include "MetalSemaphore.h"

#include <utility>

namespace rhi::metal {

Result<SemaphoreRef> MetalSemaphore::create(std::shared_ptr<MetalDeviceContext> context, SemaphoreDesc desc) {
    id<MTLSharedEvent> event = [context->device newSharedEvent];
    if (event == nil) {
        return Error{ErrorCode::InitializationFailed, "MTLDevice newSharedEvent failed"};
    }
    event.label = toNSString(desc.label);

    event.signaledValue = desc.kind == SemaphoreKind::Timeline ? desc.initial_value : 0;
    return SemaphoreRef(std::make_shared<MetalSemaphore>(std::move(context), desc.kind, event));
}

MetalSemaphore::MetalSemaphore(std::shared_ptr<MetalDeviceContext> context, SemaphoreKind kind,
                               id<MTLSharedEvent> event)
    : m_kind(kind), m_event(event), m_context(std::move(context)) {
}

SemaphoreKind MetalSemaphore::getKind() const {
    return m_kind;
}

std::uint64_t MetalSemaphore::getValue() const {
    return m_kind == SemaphoreKind::Timeline ? m_event.signaledValue : 0;
}

Result<void> MetalSemaphore::signal(std::uint64_t value) {
    if (m_kind != SemaphoreKind::Timeline) {
        return Error{ErrorCode::InvalidArgument, "ISemaphore::signal: only Timeline semaphores can be signaled"};
    }

    if (value <= m_event.signaledValue) {
        return Error{ErrorCode::InvalidArgument, "ISemaphore::signal: value must exceed the current value"};
    }
    m_event.signaledValue = value;
    return {};
}

std::uint64_t MetalSemaphore::nextSignalValue(std::uint64_t requested) {
    if (m_kind == SemaphoreKind::Timeline) {
        return requested;
    }
    return ++m_binary_signals;
}

void MetalSemaphore::signalFromHost() {
    if (m_kind == SemaphoreKind::Binary) {
        m_event.signaledValue = ++m_binary_signals;
    }
}

Result<std::uint64_t> MetalSemaphore::nextWaitValue(std::uint64_t requested) {
    if (m_kind == SemaphoreKind::Timeline) {
        return requested;
    }

    std::uint64_t waits = m_binary_waits.load();
    do {
        if (waits >= m_binary_signals.load()) {
            return Error{ErrorCode::InvalidArgument,
                         "MetalQueue::submit: waiting on a binary semaphore that no earlier submission signals"};
        }
    } while (!m_binary_waits.compare_exchange_weak(waits, waits + 1));
    return waits + 1;
}

} // namespace rhi::metal
