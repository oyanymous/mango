#pragma once

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
    BufferRef buffer;         /**< Buffer with BufferUsage::Vertex. */
    std::uint64_t offset = 0; /**< Byte offset of the first vertex. */
};

/// Binds `buffers[i]` to vertex buffer slot `first_slot + i`.
struct VertexBufferBindings {
    std::uint32_t first_slot = 0;             /**< First slot to bind. */
    std::vector<VertexBufferBinding> buffers; /**< Buffers for consecutive slots. */
};

/// The index buffer for indexed draws.
struct IndexBufferBinding {
    BufferRef buffer;                         /**< Buffer with BufferUsage::Index. */
    std::uint64_t offset = 0;                 /**< Byte offset of the first index. */
    IndexFormat format = IndexFormat::Uint32; /**< Index size. */
};

struct DrawDesc {
    std::uint32_t vertex_count = 0;
    std::uint32_t instance_count = 1;
    std::uint32_t first_vertex = 0;   /**< First vertex index. */
    std::uint32_t first_instance = 0; /**< First instance index. */
};

struct DrawIndexedDesc {
    std::uint32_t index_count = 0; /**< Indices to read. */
    std::uint32_t instance_count = 1;
    std::uint32_t first_index = 0;    /**< First index in the index buffer. */
    std::int32_t vertex_offset = 0;   /**< Added to every index before reading vertices. */
    std::uint32_t first_instance = 0; /**< First instance index. */
};

/// A rectangle of pixels in a render target, origin at the top-left corner.
struct ScissorRect {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    friend bool operator==(const ScissorRect&, const ScissorRect&) = default;
};

/// The viewport: the rectangle normalized device coordinates map to, origin at the top-left corner.
struct Viewport {
    float x = 0.0f;         /**< Left edge, in pixels. */
    float y = 0.0f;         /**< Top edge, in pixels. */
    float width = 0.0f;     /**< Width, in pixels. */
    float height = 0.0f;    /**< Height, in pixels. */
    float min_depth = 0.0f; /**< Depth NDC 0 maps to. */
    float max_depth = 1.0f; /**< Depth NDC 1 maps to. */
};

/**
 * Records draws inside a render pass. Returned by ICommandBuffer::beginRender.
 *
 * The viewport and scissor start out covering the whole render target and the stencil reference at 0; setViewport,
 * setScissor and setStencilReference change them for following draws. Typical use:
 *
 * @code
 * auto pass = cmd->beginRender({.render_pass = pass_ref, .target = target, .clear_colors = {{0, 0, 0, 1}}});
 * pass->setPipeline(pipeline);
 * pass->setBindGroup({.index = 0, .bind_group = globals});
 * pass->setVertexBuffers({.buffers = {{vertices}}});
 * pass->draw({.vertex_count = 3});
 * pass->end();
 * @endcode
 * end() must be called before the command buffer records anything else.
 */
class IRenderEncoder {
public:
    virtual ~IRenderEncoder() = default;

    /**
     * Sets the graphics pipeline for following draws. Retained by the command buffer.
     *
     * @param pipeline A PipelineKind::Graphics pipeline compatible with the current render pass.
     */
    virtual void setPipeline(PipelineRef pipeline) = 0;
    /**
     * Binds a bind group for following draws.
     *
     * @param binding Group and the index it binds to.
     */
    virtual void setBindGroup(BindGroupBinding binding) = 0;
    /**
     * Binds vertex buffers for following draws.
     *
     * @param bindings Buffers and the first slot.
     */
    virtual void setVertexBuffers(VertexBufferBindings bindings) = 0;
    /**
     * Binds the index buffer for following drawIndexed() calls.
     *
     * @param binding Buffer, offset and index format.
     */
    virtual void setIndexBuffer(IndexBufferBinding binding) = 0;
    virtual void draw(DrawDesc desc) = 0;
    /// Draws indexed primitives with the current index buffer.
    virtual void drawIndexed(DrawIndexedDesc desc) = 0;
    /**
     * Sets the viewport for following draws.
     *
     * @param viewport The viewport, in pixels of the render target.
     */
    virtual void setViewport(Viewport viewport) = 0;
    /**
     * Restricts following draws to a rectangle of the render target.
     *
     * The rectangle is clamped to the render target; a rectangle entirely outside it discards every fragment.
     *
     * @param rect The scissor rectangle, in pixels.
     */
    virtual void setScissor(ScissorRect rect) = 0;
    /**
     * Sets the reference value of the stencil test (DepthStencilState::stencil) for following draws.
     *
     * @param reference The reference value; only the low 8 bits are used.
     */
    virtual void setStencilReference(std::uint32_t reference) = 0;
    /// Ends the render pass. The encoder must not be used afterwards.
    virtual void end() = 0;
};

} // namespace rhi
