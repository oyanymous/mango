#pragma once

#include <string>
#include <vector>

#include "IRenderPass.h"
#include "ITextureView.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

// The concrete views rendered into (a Vulkan framebuffer). Must match a compatible render pass.
struct RenderTargetDesc {
    RenderPassRef render_pass;
    std::vector<TextureViewRef> color_attachments;
    TextureViewRef depth_stencil_attachment; // may be null
    Extent2D extent;
    std::string label;
};

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;

    virtual RenderTargetDesc getDesc() const = 0;
};

} // namespace rhi
