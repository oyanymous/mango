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

struct CommandBufferDesc {
    QueueType queue = QueueType::Graphics; // the queue type it will be submitted to
    std::string label;
};

// How a resource is about to be used. Barriers move resources between states; the backend
// derives pipeline stages, access masks and image layouts from them.
enum class ResourceState {
    Undefined, // contents may be discarded
    CopySrc,
    CopyDst,
    VertexBuffer,
    IndexBuffer,
    UniformBuffer,
    ShaderRead,  // sampled texture or read-only storage
    ShaderWrite, // read-write storage buffer / texture
    RenderTarget,
    DepthStencilWrite,
    DepthStencilRead,
    Present,
};

struct BufferBarrier {
    BufferRef buffer;
    ResourceState before = ResourceState::Undefined;
    ResourceState after = ResourceState::Undefined;
    std::uint64_t offset = 0;
    std::uint64_t size = kWholeSize;
};

struct TextureBarrier {
    TextureRef texture;
    ResourceState before = ResourceState::Undefined;
    ResourceState after = ResourceState::Undefined;
    TextureSubresourceRange range;
};

struct BarrierBatch {
    std::vector<BufferBarrier> buffers;
    std::vector<TextureBarrier> textures;
};

struct ClearDepthStencil {
    float depth = 1.0f;
    std::uint32_t stencil = 0;
};

struct RenderBeginDesc {
    RenderPassRef render_pass;
    RenderTargetRef target;
    // One RGBA value per colour attachment whose load op is Clear.
    std::vector<std::array<float, 4>> clear_colors;
    ClearDepthStencil clear_depth_stencil;
};

// Records GPU work. Recording happens through one scoped encoder at a time: beginX() returns an
// encoder that must be end()ed before the next beginX(), barrier() or end().
class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;

    virtual Result<void> begin() = 0;
    virtual void barrier(BarrierBatch batch) = 0;
    virtual RenderEncoderRef beginRender(RenderBeginDesc desc) = 0;
    virtual ComputeEncoderRef beginCompute() = 0;
    virtual CopyEncoderRef beginCopy() = 0;
    virtual Result<void> end() = 0;
};

} // namespace rhi
