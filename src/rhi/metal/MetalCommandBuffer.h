#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include "ICommandBuffer.h"
#include "MetalCommon.h"

namespace rhi::metal {

/**
 * A reusable RHI command buffer over one-shot `MTLCommandBuffer`s.
 *
 * Metal command buffers can be committed only once, so begin() takes a new one from the queue the RHI command buffer
 * was created for, and it must be submitted to that same queue. As in the Vulkan backend, begin() fails while the
 * previous submission is still executing. Metal tracks hazards on every resource, so barrier()
 * records no GPU work; it only retains the resources. The `MTLCommandBuffer` retains the Metal objects it uses, and
 * the queue keeps the RHI command buffer alive until it completes.
 */
class MetalCommandBuffer final : public ICommandBuffer, public std::enable_shared_from_this<MetalCommandBuffer> {
public:
    /// Where the command buffer is in its life cycle.
    enum class RecordingState {
        Initial,    /**< Created or never begun. */
        Recording,  /**< Between begin() and end(), no encoder open. */
        Encoding,   /**< An encoder is open. */
        Executable, /**< end() succeeded; can be submitted once. */
        Submitted,  /**< Committed; begin() again to reuse it. */
    };

    /**
     * Creates a command buffer for a queue.
     *
     * @param queue The `MTLCommandQueue` of `desc.queue`.
     */
    MetalCommandBuffer(std::shared_ptr<MetalDeviceContext> context, CommandBufferDesc desc, id<MTLCommandQueue> queue);

    Result<void> begin() override;
    void barrier(BarrierBatch batch) override;

    /// Opens an `MTLRenderCommandEncoder` on an `MTLRenderPassDescriptor` built from the pass and target, with the
    /// viewport and scissor set to the target's extent.
    RenderEncoderRef beginRender(RenderBeginDesc desc) override;

    /// Opens an `MTLComputeCommandEncoder`.
    ComputeEncoderRef beginCompute() override;

    /// Opens an `MTLBlitCommandEncoder`.
    CopyEncoderRef beginCopy() override;

    Result<void> end() override;

    /// The `MTLCommandBuffer` of the current recording; nil before the first begin().
    id<MTLCommandBuffer> handle() const { return m_command_buffer; }

    id<MTLCommandQueue> queue() const { return m_queue; }

    /// Where it is in its life cycle.
    RecordingState state() const { return m_state; }

    /// Called by MetalQueue::submit after committing it: begin() fails until markCompleted().
    void markSubmitted() {
        m_state = RecordingState::Submitted;
        m_pending = true;
    }

    /// Called from the submission's completion handler, on a Metal thread.
    void markCompleted() { m_pending = false; }

    /**
     * Keeps a resource alive until the next begin().
     *
     * @param resource Any RHI object used by the recording.
     */
    void retain(std::shared_ptr<const void> resource);

    /// Called by an encoder's end(): back to RecordingState::Recording.
    void endEncoder();

    /**
     * Reports invalid recording (e.g. an unimplemented command) as DebugSeverity::Error.
     *
     * @param message What went wrong.
     */
    void reportError(std::string_view message) const;
private:
    CommandBufferDesc m_desc;
    RecordingState m_state = RecordingState::Initial;
    std::atomic<bool> m_pending{false}; // submitted and not yet completed
    std::vector<std::shared_ptr<const void>> m_retained_resources;
    id<MTLCommandQueue> m_queue = nil;
    id<MTLCommandBuffer> m_command_buffer = nil;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
