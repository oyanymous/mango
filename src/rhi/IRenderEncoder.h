#pragma once

/// @file
/// rhi::IRenderEncoder: records draws inside a render pass.

#include <cstdint>
#include <vector>

#include "IBindGroup.h"
#include "IBuffer.h"
#include "IPipeline.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

/// A vertex buffer bound to one slot.
struct VertexBufferBinding {
    BufferRef buffer;         ///< Buffer with BufferUsage::Vertex.
    std::uint64_t offset = 0; ///< Byte offset of the first vertex.
};

/// Binds `buffers[i]` to vertex buffer slot `first_slot + i`.
struct VertexBufferBindings {
    std::uint32_t first_slot = 0;             ///< First slot to bind.
    std::vector<VertexBufferBinding> buffers; ///< Buffers for consecutive slots.
};

/// The index buffer for indexed draws.
struct IndexBufferBinding {
    BufferRef buffer;                         ///< Buffer with BufferUsage::Index.
    std::uint64_t offset = 0;                 ///< Byte offset of the first index.
    IndexFormat format = IndexFormat::Uint32; ///< Index size.
};

/// A non-indexed draw.
struct DrawDesc {
    std::uint32_t vertex_count = 0;   ///< Vertices to draw.
    std::uint32_t instance_count = 1; ///< Instances to draw.
    std::uint32_t first_vertex = 0;   ///< First vertex index.
    std::uint32_t first_instance = 0; ///< First instance index.
};

/// An indexed draw.
struct DrawIndexedDesc {
    std::uint32_t index_count = 0;    ///< Indices to read.
    std::uint32_t instance_count = 1; ///< Instances to draw.
    std::uint32_t first_index = 0;    ///< First index in the index buffer.
    std::int32_t vertex_offset = 0;   ///< Added to every index before reading vertices.
    std::uint32_t first_instance = 0; ///< First instance index.
};

/// Records draws inside a render pass. Returned by ICommandBuffer::beginRender.
///
/// The viewport and scissor cover the whole render target. Typical use:
/// @code
/// auto pass = cmd->beginRender({.render_pass = pass_ref, .target = target, .clear_colors = {{0, 0, 0, 1}}});
/// pass->setPipeline(pipeline);
/// pass->setBindGroup({.index = 0, .bind_group = globals});
/// pass->setVertexBuffers({.buffers = {{vertices}}});
/// pass->draw({.vertex_count = 3});
/// pass->end();
/// @endcode
/// end() must be called before the command buffer records anything else.
class IRenderEncoder {
public:
    virtual ~IRenderEncoder() = default;

    /// Sets the graphics pipeline for following draws. Retained by the command buffer.
    /// @param pipeline A PipelineKind::Graphics pipeline compatible with the current render pass.
    virtual void setPipeline(PipelineRef pipeline) = 0;
    /// Binds a bind group for following draws.
    /// @param binding Group and the index it binds to.
    virtual void setBindGroup(BindGroupBinding binding) = 0;
    /// Binds vertex buffers for following draws.
    /// @param bindings Buffers and the first slot.
    virtual void setVertexBuffers(VertexBufferBindings bindings) = 0;
    /// Binds the index buffer for following drawIndexed() calls.
    /// @param binding Buffer, offset and index format.
    virtual void setIndexBuffer(IndexBufferBinding binding) = 0;
    /// Draws non-indexed primitives.
    /// @param desc Vertex and instance ranges.
    virtual void draw(DrawDesc desc) = 0;
    /// Draws indexed primitives with the current index buffer.
    /// @param desc Index and instance ranges.
    virtual void drawIndexed(DrawIndexedDesc desc) = 0;
    /// Ends the render pass. The encoder must not be used afterwards.
    virtual void end() = 0;
};

} // namespace rhi
