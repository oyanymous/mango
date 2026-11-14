/**
 * @file
 * @brief MetalCommandBuffer: recording state, resource retention and encoder creation.
 */

#include "MetalCommandBuffer.h"

#include <array>
#include <utility>

#include "MetalComputeEncoder.h"
#include "MetalCopyEncoder.h"
#include "MetalRenderEncoder.h"
#include "MetalTexture.h"

namespace rhi::metal {

namespace {

/**
 * @brief The Metal load action of a LoadOp.
 */
MTLLoadAction toMTLLoadAction(LoadOp op) {
    switch (op) {
    case LoadOp::Load:
        return MTLLoadActionLoad;
    case LoadOp::Clear:
        return MTLLoadActionClear;
    case LoadOp::DontCare:
        return MTLLoadActionDontCare;
    }
    return MTLLoadActionDontCare;
}

/**
 * @brief The Metal store action of a StoreOp.
 */
MTLStoreAction toMTLStoreAction(StoreOp op) {
    return op == StoreOp::Store ? MTLStoreActionStore : MTLStoreActionDontCare;
}

} // namespace

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

RenderEncoderRef MetalCommandBuffer::beginRender(RenderBeginDesc desc) {
    if (m_state != RecordingState::Recording || !desc.render_pass || !desc.target) {
        return nullptr;
    }
    const RenderPassDesc pass_desc = desc.render_pass->getDesc();
    const RenderTargetDesc target_desc = desc.target->getDesc();
    const Extent2D extent = target_desc.extent;

    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    std::size_t next_clear = 0;
    for (std::size_t i = 0; i < pass_desc.color_attachments.size(); ++i) {
        const AttachmentDesc& attachment = pass_desc.color_attachments[i];
        MTLRenderPassColorAttachmentDescriptor* color = pass.colorAttachments[i];
        color.texture = static_cast<const MetalTextureView&>(*target_desc.color_attachments[i]).handle();
        color.loadAction = toMTLLoadAction(attachment.load_op);
        color.storeAction = toMTLStoreAction(attachment.store_op);
        // Clear colours are given only for attachments that clear, in attachment order (as in the Vulkan backend).
        if (attachment.load_op == LoadOp::Clear && next_clear < desc.clear_colors.size()) {
            const std::array<float, 4>& value = desc.clear_colors[next_clear++];
            color.clearColor = MTLClearColorMake(value[0], value[1], value[2], value[3]);
        }
    }
    if (pass_desc.depth_stencil_attachment && target_desc.depth_stencil_attachment) {
        const AttachmentDesc& attachment = *pass_desc.depth_stencil_attachment;
        id<MTLTexture> texture = static_cast<const MetalTextureView&>(*target_desc.depth_stencil_attachment).handle();
        pass.depthAttachment.texture = texture;
        pass.depthAttachment.loadAction = toMTLLoadAction(attachment.load_op);
        pass.depthAttachment.storeAction = toMTLStoreAction(attachment.store_op);
        pass.depthAttachment.clearDepth = desc.clear_depth_stencil.depth;
        if (hasStencil(attachment.format)) {
            pass.stencilAttachment.texture = texture;
            pass.stencilAttachment.loadAction = toMTLLoadAction(attachment.stencil_load_op);
            pass.stencilAttachment.storeAction = toMTLStoreAction(attachment.stencil_store_op);
            pass.stencilAttachment.clearStencil = desc.clear_depth_stencil.stencil;
        }
    }
    pass.renderTargetWidth = extent.width;
    pass.renderTargetHeight = extent.height;

    id<MTLRenderCommandEncoder> encoder = [m_command_buffer renderCommandEncoderWithDescriptor:pass];
    if (encoder == nil) {
        return nullptr;
    }
    encoder.label = toNSString(pass_desc.label);
    // The render area is the target's extent, which may be smaller than the attachments (as in the Vulkan backend).
    [encoder setViewport:MTLViewport{0.0, 0.0, static_cast<double>(extent.width), static_cast<double>(extent.height),
                                     0.0, 1.0}];
    [encoder setScissorRect:MTLScissorRect{0, 0, extent.width, extent.height}];

    retain(std::move(desc.render_pass));
    retain(std::move(desc.target));
    m_state = RecordingState::Encoding;
    return std::make_unique<MetalRenderEncoder>(shared_from_this(), encoder);
}

ComputeEncoderRef MetalCommandBuffer::beginCompute() {
    if (m_state != RecordingState::Recording) {
        return nullptr;
    }
    id<MTLComputeCommandEncoder> encoder = [m_command_buffer computeCommandEncoder];
    if (encoder == nil) {
        return nullptr;
    }
    m_state = RecordingState::Encoding;
    return std::make_unique<MetalComputeEncoder>(shared_from_this(), encoder);
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
