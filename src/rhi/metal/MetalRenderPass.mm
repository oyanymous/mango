/**
 * @file
 * @brief MetalRenderPass and MetalRenderTarget validation.
 */

#include "MetalRenderPass.h"

#include <utility>

namespace rhi::metal {

Result<RenderPassRef> MetalRenderPass::create(RenderPassDesc desc) {
    if (desc.color_attachments.empty() && !desc.depth_stencil_attachment) {
        return Error{ErrorCode::InvalidArgument, "createRenderPass: the pass has no attachments"};
    }
    for (const AttachmentDesc& color : desc.color_attachments) {
        if (color.format == Format::Undefined) {
            return Error{ErrorCode::InvalidArgument, "createRenderPass: colour attachment format is Undefined"};
        }
    }
    if (desc.depth_stencil_attachment && desc.depth_stencil_attachment->format == Format::Undefined) {
        return Error{ErrorCode::InvalidArgument, "createRenderPass: depth attachment format is Undefined"};
    }
    return RenderPassRef(std::make_shared<MetalRenderPass>(std::move(desc)));
}

MetalRenderPass::MetalRenderPass(RenderPassDesc desc) : m_desc(std::move(desc)) {
}

RenderPassDesc MetalRenderPass::getDesc() const {
    return m_desc;
}

Result<RenderTargetRef> MetalRenderTarget::create(RenderTargetDesc desc) {
    if (!desc.render_pass) {
        return Error{ErrorCode::InvalidArgument, "createRenderTarget: render_pass is null"};
    }
    const RenderPassDesc pass_desc = desc.render_pass->getDesc();
    if (desc.color_attachments.size() != pass_desc.color_attachments.size() ||
        static_cast<bool>(desc.depth_stencil_attachment) != pass_desc.depth_stencil_attachment.has_value()) {
        return Error{ErrorCode::InvalidArgument, "createRenderTarget: attachments do not match the render pass"};
    }
    if (desc.extent.width == 0 || desc.extent.height == 0) {
        return Error{ErrorCode::InvalidArgument, "createRenderTarget: extent must be non-zero"};
    }
    for (const TextureViewRef& view : desc.color_attachments) {
        if (!view) {
            return Error{ErrorCode::InvalidArgument, "createRenderTarget: null colour attachment"};
        }
    }
    return RenderTargetRef(std::make_shared<MetalRenderTarget>(std::move(desc)));
}

MetalRenderTarget::MetalRenderTarget(RenderTargetDesc desc) : m_desc(std::move(desc)) {
}

RenderTargetDesc MetalRenderTarget::getDesc() const {
    return m_desc;
}

} // namespace rhi::metal
