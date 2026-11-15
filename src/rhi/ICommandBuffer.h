#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "IBuffer.h"
#include "IComputeEncoder.h"
#include "ICopyEncoder.h"
#include "IRenderEncoder.h"
#include "IRenderPass.h"
#include "IRenderTarget.h"
#include "ITexture.h"
#include "RHIFwd.h"
#include "RHITypes.h"
#include "Result.h"

namespace rhi {

/// Describes a command buffer to create with IDevice::createCommandBuffer.
struct CommandBufferDesc {
    QueueType queue = QueueType::Graphics; /**< The queue type it will be submitted to. */
    std::string label;
};

/**
 * How a resource is about to be used.
 *
 * Barriers move resources between states; the backend derives pipeline stages, memory access and image
 * layouts from them, so callers never deal with those directly.
 */
enum class ResourceState {
    Undefined,         /**< Contents may be discarded. The initial state of every texture. */
    CopySrc,           /**< Source of a copy. */
    CopyDst,           /**< Destination of a copy. */
    VertexBuffer,      /**< Read as vertex data. */
    IndexBuffer,       /**< Read as index data. */
    UniformBuffer,     /**< Read as a uniform buffer. */
    ShaderRead,        /**< Sampled texture or read-only storage in shaders. */
    ShaderWrite,       /**< Read-write storage buffer or texture in shaders. */
    RenderTarget,      /**< Colour attachment. */
    DepthStencilWrite, /**< Depth/stencil attachment with writes. */
    DepthStencilRead,  /**< Depth/stencil attachment, read-only (or sampled depth). */
    Present,           /**< Ready for IQueue::present (swapchain images only). */
};

/// A state transition of (part of) a buffer.
struct BufferBarrier {
    BufferRef buffer;
    ResourceState before = ResourceState::Undefined; /**< State it was used in. */
    ResourceState after = ResourceState::Undefined;  /**< State it will be used in. */
    std::uint64_t offset = 0;                        /**< First byte affected. */
    std::uint64_t size = kWholeSize;                 /**< Bytes affected. */
};

/// A state transition of (part of) a texture.
struct TextureBarrier {
    TextureRef texture;
    ResourceState before = ResourceState::Undefined; /**< State it was used in; Undefined discards contents. */
    ResourceState after = ResourceState::Undefined;  /**< State it will be used in. */
    TextureSubresourceRange range;                   /**< Mip levels and layers affected. */
};

/// Barriers recorded together; batching them is cheaper than one barrier() call each.
struct BarrierBatch {
    std::vector<BufferBarrier> buffers;
    std::vector<TextureBarrier> textures;
};

/// Clear values for a depth/stencil attachment.
struct ClearDepthStencil {
    float depth = 1.0f;        /**< Depth clear value (1 = far plane). */
    std::uint32_t stencil = 0; /**< Stencil clear value. */
};

/// Parameters for ICommandBuffer::beginRender.
struct RenderBeginDesc {
    RenderPassRef render_pass; /**< Load/store semantics of the attachments. */
    RenderTargetRef target;    /**< Views to render into; must be compatible with `render_pass`. */
    /// One RGBA value per colour attachment whose load op is LoadOp::Clear, in attachment order.
    std::vector<std::array<float, 4>> clear_colors;
    ClearDepthStencil clear_depth_stencil; /**< Used if the depth/stencil attachment's load op is Clear. */
};

/**
 * Records GPU work for later submission with IQueue::submit.
 *
 * Recording goes through one scoped encoder at a time: each beginX() returns an encoder that must be
 * end()ed before the next beginX(), barrier() or end(). The command buffer retains every resource
 * recorded into it until the submission finishes.
 *
 * @code
 * cmd->begin();
 * cmd->barrier({.textures = {{.texture = color, .before = ResourceState::Undefined,
 *                             .after = ResourceState::RenderTarget}}});
 * auto pass = cmd->beginRender(begin_desc);
 * // ... draws ...
 * pass->end();
 * cmd->end();
 * queue->submit({.command_buffers = {cmd}});
 * @endcode
 */
class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;

    /**
     * Starts recording, discarding anything recorded before.
     *
     * @return InvalidArgument while an encoder is open or the previous submission is still executing.
     */
    virtual Result<void> begin() = 0;
    /// Records resource state transitions.
    virtual void barrier(BarrierBatch batch) = 0;
    virtual RenderEncoderRef beginRender(RenderBeginDesc desc) = 0;
    virtual ComputeEncoderRef beginCompute() = 0;
    virtual CopyEncoderRef beginCopy() = 0;
    /// Finishes recording; the command buffer can then be submitted.
    virtual Result<void> end() = 0;
};

} // namespace rhi
