#pragma once

#include <memory>

#include "IRenderPass.h"
#include "IRenderTarget.h"
#include "MetalCommon.h"

namespace rhi::metal {

/**
 * A render pass: attachment formats and load/store ops.
 *
 * Metal has no render pass object; MetalCommandBuffer::beginRender combines this with a MetalRenderTarget into an
 * `MTLRenderPassDescriptor`.
 */
class MetalRenderPass final : public IRenderPass {
public:
    /**
     * Validates and stores a render pass description.
     *
     * @return The pass, or InvalidArgument for a pass without attachments or with an Undefined format.
     */
    static Result<RenderPassRef> create(RenderPassDesc desc);

    /// Wraps a validated description. Use create().
    explicit MetalRenderPass(RenderPassDesc desc);

    RenderPassDesc getDesc() const override;
private:
    RenderPassDesc m_desc;
};

/// The texture views a render pass draws into. Metal has no framebuffer object, so this only validates and retains
/// them.
class MetalRenderTarget final : public IRenderTarget {
public:
    /**
     * Validates and stores a render target description.
     *
     * @return The target, or InvalidArgument if the views do not match the pass or the extent is zero.
     */
    static Result<RenderTargetRef> create(RenderTargetDesc desc);

    /// Wraps a validated description. Use create().
    explicit MetalRenderTarget(RenderTargetDesc desc);

    RenderTargetDesc getDesc() const override;
private:
    RenderTargetDesc m_desc;
};

} // namespace rhi::metal
