#pragma once

#include <cstdint>
#include <vector>

#include "IBindGroup.h"
#include "IBuffer.h"
#include "IPipeline.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

struct VertexBufferBinding {
    BufferRef buffer;
    std::uint64_t offset = 0;
};

struct VertexBufferBindings {
    std::uint32_t first_slot = 0;
    std::vector<VertexBufferBinding> buffers;
};

struct IndexBufferBinding {
    BufferRef buffer;
    std::uint64_t offset = 0;
    IndexFormat format = IndexFormat::Uint32;
};

struct DrawDesc {
    std::uint32_t vertex_count = 0;
    std::uint32_t instance_count = 1;
    std::uint32_t first_vertex = 0;
    std::uint32_t first_instance = 0;
};

struct DrawIndexedDesc {
    std::uint32_t index_count = 0;
    std::uint32_t instance_count = 1;
    std::uint32_t first_index = 0;
    std::int32_t vertex_offset = 0;
    std::uint32_t first_instance = 0;
};

struct ScissorRect {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    friend bool operator==(const ScissorRect&, const ScissorRect&) = default;
};

struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float min_depth = 0.0f;
    float max_depth = 1.0f;
};

class IRenderEncoder {
public:
    virtual ~IRenderEncoder() = default;

    virtual void setPipeline(PipelineRef pipeline) = 0;

    virtual void setBindGroup(BindGroupBinding binding) = 0;

    virtual void setVertexBuffers(VertexBufferBindings bindings) = 0;

    virtual void setIndexBuffer(IndexBufferBinding binding) = 0;
    virtual void draw(DrawDesc desc) = 0;

    virtual void drawIndexed(DrawIndexedDesc desc) = 0;

    virtual void setViewport(Viewport viewport) = 0;

    virtual void setScissor(ScissorRect rect) = 0;

    virtual void setStencilReference(std::uint32_t reference) = 0;

    virtual void end() = 0;
};

} // namespace rhi
