#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

enum class LoadOp {
    Load,
    Clear,
    DontCare,
};

enum class StoreOp {
    Store,
    DontCare,
};

struct AttachmentDesc {
    Format format = Format::Undefined;
    std::uint32_t sample_count = 1;
    LoadOp load_op = LoadOp::Clear;
    StoreOp store_op = StoreOp::Store;
    // Depth/stencil attachments only.
    LoadOp stencil_load_op = LoadOp::DontCare;
    StoreOp stencil_store_op = StoreOp::DontCare;
};

// Attachment formats and load/store semantics. Attachments must be in the RenderTarget /
// DepthStencilWrite state (see ResourceState) when rendering begins, and stay in it.
struct RenderPassDesc {
    std::vector<AttachmentDesc> color_attachments;
    std::optional<AttachmentDesc> depth_stencil_attachment;
    std::string label;
};

class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    virtual RenderPassDesc getDesc() const = 0;
};

} // namespace rhi
