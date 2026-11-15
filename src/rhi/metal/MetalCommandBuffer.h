#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include "ICommandBuffer.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalCommandBuffer final : public ICommandBuffer, public std::enable_shared_from_this<MetalCommandBuffer> {
public:
    enum class RecordingState {
        Initial,
        Recording,
        Encoding,
        Executable,
        Submitted,
    };

    MetalCommandBuffer(std::shared_ptr<MetalDeviceContext> context, CommandBufferDesc desc, id<MTLCommandQueue> queue);

    Result<void> begin() override;
    void barrier(BarrierBatch batch) override;

    RenderEncoderRef beginRender(RenderBeginDesc desc) override;

    ComputeEncoderRef beginCompute() override;

    CopyEncoderRef beginCopy() override;

    Result<void> end() override;

    id<MTLCommandBuffer> handle() const { return m_command_buffer; }

    id<MTLCommandQueue> queue() const { return m_queue; }

    RecordingState state() const { return m_state; }

    void markSubmitted() {
        m_state = RecordingState::Submitted;
        m_pending = true;
    }

    void markCompleted() { m_pending = false; }

    void retain(std::shared_ptr<const void> resource);

    void endEncoder();

    void reportError(std::string_view message) const;
private:
    CommandBufferDesc m_desc;
    RecordingState m_state = RecordingState::Initial;
    std::atomic<bool> m_pending{false};
    std::vector<std::shared_ptr<const void>> m_retained_resources;
    id<MTLCommandQueue> m_queue = nil;
    id<MTLCommandBuffer> m_command_buffer = nil;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
