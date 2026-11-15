#pragma once

#include <string>
#include <vector>

#include "IRenderPass.h"
#include "ITextureView.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

struct RenderTargetDesc {
    RenderPassRef render_pass;
    std::vector<TextureViewRef> color_attachments;
    TextureViewRef depth_stencil_attachment;
    Extent2D extent;
    std::string label;
};

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;

    virtual RenderTargetDesc getDesc() const = 0;
};

} // namespace rhi
