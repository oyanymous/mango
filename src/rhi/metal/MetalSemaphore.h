#pragma once

/**
 * @file
 * @brief rhi::metal::MetalSemaphore: GPU-GPU ordering on an `MTLSharedEvent`.
 */

#include <atomic>
#include <cstdint>
#include <memory>

#include "ISemaphore.h"
#include "MetalCommon.h"
#include "Result.h"

namespace rhi::metal {

/**
 * @brief A semaphore backed by an `MTLSharedEvent`, whose 64-bit value only increases.
 *
 * @details
 * A Timeline semaphore uses the event value directly. A Binary semaphore keeps two counters: each signal sets the
 * event to the next signal number, and each wait waits for the next wait number, so the n-th wait pairs with the
 * n-th signal. The counters are atomic, so submissions from several threads are safe.
 */
class MetalSemaphore final : public ISemaphore {
public:
    /**
     * @brief Creates the event.
     *
     * @param context Device context; kept alive by the semaphore.
     * @param desc Kind, initial value and label.
     */
    static Result<SemaphoreRef> create(std::shared_ptr<MetalDeviceContext> context, SemaphoreDesc desc);

    /**
     * @brief Wraps an event. Use create().
     *
     * @param context Device context.
     * @param kind Binary or Timeline.
     * @param event The event, already set to its initial value.
     */
    MetalSemaphore(std::shared_ptr<MetalDeviceContext> context, SemaphoreKind kind, id<MTLSharedEvent> event);

    SemaphoreKind getKind() const override;
    std::uint64_t getValue() const override;

    /**
     * @brief The event value a submission signaling @p requested must set. Timeline: @p requested; Binary: the next
     * signal number (@p requested is ignored).
     */
    std::uint64_t nextSignalValue(std::uint64_t requested);

    /**
     * @brief The event value a submission waiting for @p requested must wait for. Timeline: @p requested; Binary: the
     * next wait number.
     *
     * @return The value, or ErrorCode::InvalidArgument for a Binary semaphore no earlier submission signals (the GPU
     *         would wait forever).
     */
    Result<std::uint64_t> nextWaitValue(std::uint64_t requested);

    /**
     * @brief The event to encode waits and signals on.
     */
    id<MTLSharedEvent> event() const { return m_event; }
private:
    SemaphoreKind m_kind;
    id<MTLSharedEvent> m_event = nil;
    std::atomic<std::uint64_t> m_binary_signals{0}; // signals submitted so far (Binary only)
    std::atomic<std::uint64_t> m_binary_waits{0};   // waits submitted so far (Binary only)
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
