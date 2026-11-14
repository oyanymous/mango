/**
 * @file
 * @brief MetalCommandBuffer: recording state, resource retention and encoder creation.
 */

#include "MetalCommandBuffer.h"

#include <utility>

#include "MetalCopyEncoder.h"

namespace rhi::metal {

MetalCommandBuffer::MetalCommandBuffer(std::shared_ptr<MetalDeviceContext> context, CommandBufferDesc desc,
                                       id<MTLCommandQueue> queue)
    : m_desc(std::move(desc)), m_queue(queue), m_context(std::move(context)) {
}

Result<void> MetalCommandBuffer::begin() {
    if (m_state == RecordingState::Encoding) {
        return Error{ErrorCode::InvalidArgument, "ICommandBuffer::begin: an encoder is still open"};
    }
    if (m_pending) {
        return Error{ErrorCode::InvalidArgument, "ICommandBuffer::begin: the previous submission is still executing"};
    }
    m_retained_resources.clear();
    m_command_buffer = [m_queue commandBuffer];
    if (m_command_buffer == nil) {
        m_state = RecordingState::Initial;
        return Error{ErrorCode::OutOfHostMemory, "MTLCommandQueue commandBuffer failed"};
    }
    m_command_buffer.label = toNSString(m_desc.label);
    m_state = RecordingState::Recording;
    return {};
}

void MetalCommandBuffer::barrier(BarrierBatch batch) {
    if (m_state != RecordingState::Recording) {
        return;
    }
    // Metal's automatic hazard tracking orders the accesses; keep the resources alive like the Vulkan backend does.
    for (BufferBarrier& b : batch.buffers) {
        if (b.buffer) {
            retain(std::move(b.buffer));
        }
    }
    for (TextureBarrier& t : batch.textures) {
        if (t.texture) {
            retain(std::move(t.texture));
        }
    }
}

RenderEncoderRef MetalCommandBuffer::beginRender(RenderBeginDesc) {
    return nullptr;
}

ComputeEncoderRef MetalCommandBuffer::beginCompute() {
    return nullptr;
}

CopyEncoderRef MetalCommandBuffer::beginCopy() {
    if (m_state != RecordingState::Recording) {
        return nullptr;
    }
    id<MTLBlitCommandEncoder> encoder = [m_command_buffer blitCommandEncoder];
    if (encoder == nil) {
        return nullptr;
    }
    m_state = RecordingState::Encoding;
    return std::make_unique<MetalCopyEncoder>(shared_from_this(), encoder);
}

Result<void> MetalCommandBuffer::end() {
    if (m_state != RecordingState::Recording) {
        return Error{ErrorCode::InvalidArgument, "ICommandBuffer::end: not recording, or an encoder is still open"};
    }
    // Shared memory is coherent once the command buffer completes, so CPU reads after the fence need no barrier.
    m_state = RecordingState::Executable;
    return {};
}

void MetalCommandBuffer::retain(std::shared_ptr<const void> resource) {
    m_retained_resources.push_back(std::move(resource));
}

void MetalCommandBuffer::endEncoder() {
    if (m_state == RecordingState::Encoding) {
        m_state = RecordingState::Recording;
    }
}

void MetalCommandBuffer::reportError(std::string_view message) const {
    m_context->reporter->report(DebugSeverity::Error, message);
}

} // namespace rhi::metal
