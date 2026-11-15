#pragma once

#include <memory>

#include "IRenderPass.h"
#include "IRenderTarget.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalRenderPass final : public IRenderPass {
public:
    static Result<RenderPassRef> create(RenderPassDesc desc);

    explicit MetalRenderPass(RenderPassDesc desc);

    RenderPassDesc getDesc() const override;
private:
    RenderPassDesc m_desc;
};

class MetalRenderTarget final : public IRenderTarget {
public:
    static Result<RenderTargetRef> create(RenderTargetDesc desc);

    explicit MetalRenderTarget(RenderTargetDesc desc);

    RenderTargetDesc getDesc() const override;
private:
    RenderTargetDesc m_desc;
};

} // namespace rhi::metal
