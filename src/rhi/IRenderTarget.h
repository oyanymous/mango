#pragma once

#include <string>
#include <vector>

#include "IRenderPass.h"
#include "ITextureView.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

/// Describes a render target to create with IDevice::createRenderTarget.
struct RenderTargetDesc {
    RenderPassRef render_pass;                     /**< Pass the attachments must match; retained. */
    std::vector<TextureViewRef> color_attachments; /**< One view per RenderPassDesc::color_attachments; retained. */
    TextureViewRef depth_stencil_attachment;       /**< Depth/stencil view, or null if the pass has none. */
    Extent2D extent; /**< Size of the render area; every attachment must be at least this big. */
    std::string label;
};

/**
 * The concrete views rendered into (a Vulkan framebuffer). Retains its attachments.
 *
 * Usable with any render pass compatible with the one it was created with (see IRenderPass).
 */
class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;

    virtual RenderTargetDesc getDesc() const = 0;
};

} // namespace rhi
