#pragma once

/**
 * @file
 * @brief rhi::metal::MetalRenderPass and MetalRenderTarget.
 */

#include <memory>

#include "IRenderPass.h"
#include "IRenderTarget.h"
#include "MetalCommon.h"

namespace rhi::metal {

/**
 * @brief A render pass: attachment formats and load/store ops.
 *
 * @details
 * Metal has no render pass object; MetalCommandBuffer::beginRender combines this with a MetalRenderTarget into an
 * `MTLRenderPassDescriptor`.
 */
class MetalRenderPass final : public IRenderPass {
public:
    /**
     * @brief Validates and stores a render pass description.
     *
     * @param desc Attachments.
     * @return The pass, or InvalidArgument for a pass without attachments or with an Undefined format.
     */
    static Result<RenderPassRef> create(RenderPassDesc desc);

    /**
     * @brief Wraps a validated description. Use create().
     *
     * @param desc The descriptor.
     */
    explicit MetalRenderPass(RenderPassDesc desc);

    RenderPassDesc getDesc() const override;
private:
    RenderPassDesc m_desc;
};

/**
 * @brief The texture views a render pass draws into. Metal has no framebuffer object, so this only validates and
 * retains them.
 */
class MetalRenderTarget final : public IRenderTarget {
public:
    /**
     * @brief Validates and stores a render target description.
     *
     * @param desc Pass, views and extent.
     * @return The target, or InvalidArgument if the views do not match the pass or the extent is zero.
     */
    static Result<RenderTargetRef> create(RenderTargetDesc desc);

    /**
     * @brief Wraps a validated description. Use create().
     *
     * @param desc The descriptor.
     */
    explicit MetalRenderTarget(RenderTargetDesc desc);

    RenderTargetDesc getDesc() const override;
private:
    RenderTargetDesc m_desc;
};

} // namespace rhi::metal
