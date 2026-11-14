#pragma once

/// @file
/// rhi::IRenderPass: attachment formats and load/store behaviour of a render pass.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

/// What happens to an attachment's contents when rendering begins.
enum class LoadOp {
    Load,     ///< Keep the existing contents.
    Clear,    ///< Clear to the value in RenderBeginDesc.
    DontCare, ///< Contents are undefined; fastest when every pixel is overwritten.
};

/// What happens to an attachment's contents when rendering ends.
enum class StoreOp {
    Store,    ///< Keep the rendered contents.
    DontCare, ///< Contents may be discarded (e.g. a depth buffer that is not read later).
};

/// One attachment of a render pass.
struct AttachmentDesc {
    Format format = Format::Undefined;            ///< Format of the attachment's texture view.
    std::uint32_t sample_count = 1;               ///< MSAA samples; must match the texture.
    LoadOp load_op = LoadOp::Clear;               ///< Colour/depth load behaviour.
    StoreOp store_op = StoreOp::Store;            ///< Colour/depth store behaviour.
    LoadOp stencil_load_op = LoadOp::DontCare;    ///< Stencil load behaviour (depth/stencil attachments only).
    StoreOp stencil_store_op = StoreOp::DontCare; ///< Stencil store behaviour (depth/stencil attachments only).
};

/// Describes a render pass to create with IDevice::createRenderPass.
///
/// Attachments must be in the ResourceState::RenderTarget / ResourceState::DepthStencilWrite state when
/// rendering begins, and stay in it; use ICommandBuffer::barrier before and after.
struct RenderPassDesc {
    std::vector<AttachmentDesc> color_attachments;          ///< Colour attachments, in location order.
    std::optional<AttachmentDesc> depth_stencil_attachment; ///< Optional depth/stencil attachment.
    std::string label;                                      ///< Debug name.
};

/// Attachment formats and load/store semantics (a Vulkan render pass).
///
/// Graphics pipelines and render targets are created against a render pass and only work with compatible
/// ones (same formats and sample counts).
class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    /// The descriptor the render pass was created with.
    virtual RenderPassDesc getDesc() const = 0;
};

} // namespace rhi
