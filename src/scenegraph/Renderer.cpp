/**
 * @file
 * @brief sg::Renderer: GPU pools, batching, damage tracking, layers, stencil clips and presentation.
 */

#include "Renderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <map>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>

#include "ClipNode.h"
#include "DamageRegion.h"
#include "GeometryNode.h"
#include "LayerNode.h"
#include "RangeAllocator.h"
#include "UpdatePass.h"

#if defined(MANGO_SCENEGRAPH_SHADERS)
#include "color.frag.h"
#include "composite.frag.h"
#include "composite.vert.h"
#include "node.vert.h"
#include "texture.frag.h"
#endif

/** @brief Evaluates a Result-returning expression and returns its error from the enclosing function. */
#define SG_TRY(expr)                                                                                                   \
    do {                                                                                                               \
        if (auto sg_try_result = (expr); !sg_try_result) {                                                             \
            return sg_try_result.error();                                                                              \
        }                                                                                                              \
    } while (false)

/** @brief Assigns the value of a Result-returning expression to @p lhs, or returns its error. */
#define SG_ASSIGN(lhs, expr)                                                                                           \
    do {                                                                                                               \
        auto sg_assign_result = (expr);                                                                                \
        if (!sg_assign_result) {                                                                                       \
            return sg_assign_result.error();                                                                           \
        }                                                                                                              \
        lhs = std::move(sg_assign_result).value();                                                                     \
    } while (false)

namespace sg {

namespace {

constexpr std::size_t kFramesInFlight = 2;
constexpr rhi::Format kCanvasFormat = rhi::Format::RGBA8Unorm;
constexpr rhi::Format kSwapchainFormat = rhi::Format::BGRA8Unorm;
constexpr std::uint32_t kBackgroundSlot = 0;           // slot 0 holds the clear colour
constexpr std::uint32_t kBackgroundVertexCount = 3;    // vertices 0..2: a triangle covering any target
constexpr std::uint32_t kClipVertexCount = 6;          // a stencil clip's rectangle, as a triangle list
constexpr std::uint32_t kLayerTextureGranularity = 64; // layer textures grow in steps, to avoid reallocating

/** @brief A vertex as the GPU sees it: Vertex plus the node's slot (24 bytes). */
struct GpuVertex {
    float x, y, u, v;
    std::uint32_t rgba; // premultiplied
    std::uint32_t slot;
};
static_assert(sizeof(GpuVertex) == 24);

/** @brief One node's slot in the node storage buffer (std430, 48 bytes). */
struct NodeData {
    float matrix[4];              // a, b, c, d
    float translation_opacity[4]; // tx, ty, opacity, unused
    float color[4];               // premultiplied
};
static_assert(sizeof(NodeData) == 48);

/** @brief The view uniform: a render target's pixel origin and 2 / size. */
struct ViewData {
    float origin_x, origin_y, scale_x, scale_y;
};

/** @brief Premultiplies a straight-alpha RGBA8 colour packed with packRgba8. */
std::uint32_t premultiply(std::uint32_t rgba) {
    const std::uint32_t a = rgba >> 24;
    const auto channel = [a](std::uint32_t c) { return (c * a + 127) / 255; };
    return channel(rgba & 0xff) | (channel((rgba >> 8) & 0xff) << 8) | (channel((rgba >> 16) & 0xff) << 16) | (a << 24);
}

/** @brief A slot's contents. */
NodeData nodeData(const Transform2D& m, float opacity, ColorF color) {
    return {{m.a, m.b, m.c, m.d},
            {m.tx, m.ty, opacity, 0.0f},
            {color.r * color.a, color.g * color.a, color.b * color.a, color.a}};
}

/** @brief The next power of two >= @p value (and >= @p minimum). */
std::uint64_t growTo(std::uint64_t value, std::uint64_t minimum) {
    std::uint64_t size = minimum;
    while (size < value) {
        size *= 2;
    }
    return size;
}

/** @brief @p value rounded up to a multiple of @p alignment. */
std::uint64_t alignUp(std::uint64_t value, std::uint64_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

/**
 * @brief A GPU-only buffer with a CPU mirror; writes go to the mirror and are uploaded as dirty byte ranges.
 */
struct GpuPool {
    rhi::BufferUsage usage = rhi::BufferUsage::None;
    rhi::ResourceState read_state = rhi::ResourceState::Undefined;
    std::string label;
    std::vector<std::byte> mirror;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> dirty; // [begin, end) byte ranges
    rhi::BufferRef buffer;
    std::uint64_t buffer_size = 0;
    bool used = false;            // the GPU has read the buffer (so a barrier from read_state is needed)
    std::uint64_t generation = 0; // bumped when the buffer is replaced (bind groups referencing it are stale)

    /** @brief Makes the mirror at least @p bytes long; the GPU buffer follows at the next upload. */
    void reserve(std::uint64_t bytes) {
        if (mirror.size() < bytes) {
            mirror.resize(growTo(bytes, 256));
        }
    }
    /** @brief Copies @p size bytes into the mirror; marks them dirty only if they changed. @return Whether they did. */
    bool write(std::uint64_t offset, const void* data, std::uint64_t size) {
        reserve(offset + size);
        if (std::memcmp(mirror.data() + offset, data, size) == 0) {
            return false;
        }
        std::memcpy(mirror.data() + offset, data, size);
        dirty.emplace_back(offset, offset + size);
        return true;
    }
    /** @brief Marks every byte dirty. */
    void markAllDirty() {
        dirty.clear();
        if (!mirror.empty()) {
            dirty.emplace_back(0, mirror.size());
        }
    }
    /** @brief Sorts and merges the dirty ranges (gaps under 256 bytes are uploaded too, for fewer copies). */
    void coalesce() {
        if (buffer_size < mirror.size()) {
            markAllDirty(); // the buffer will be recreated: upload everything
            return;
        }
        std::ranges::sort(dirty);
        std::vector<std::pair<std::uint64_t, std::uint64_t>> merged;
        for (const auto& range : dirty) {
            if (!merged.empty() && range.first <= merged.back().second + 256) {
                merged.back().second = std::max(merged.back().second, range.second);
            } else {
                merged.push_back(range);
            }
        }
        dirty = std::move(merged);
    }
};

/** @brief Key of a content pipeline. */
struct PipelineKey {
    bool lines = false;
    bool textured = false;
    bool stencil = false;
    auto operator<=>(const PipelineKey&) const = default;
};

struct Target;

/** @brief What the renderer keeps for each GeometryNode and LayerNode it draws. */
struct DrawItem {
    Node* node = nullptr;
    Target* target = nullptr; // where it is drawn
    std::uint32_t slot = 0;
    std::uint32_t vertex_offset = 0;
    std::uint32_t vertex_count = 0;
    std::vector<std::uint32_t> indices; // list indices, local to the node's vertices
    bool lines = false;
    rhi::BindGroupRef texture_group; // null: colour pipeline
    const ClipContext* clip = nullptr;
    IntRect bounds; // last drawn pixels in the target; empty when invisible
    std::uint32_t index_pos = 0;
};

/** @brief A run of consecutive items drawn with one indexed draw. */
struct Batch {
    bool lines = false;
    rhi::BindGroupRef texture_group;
    const ClipContext* clip = nullptr;
    std::uint32_t first_index = 0;
    std::uint32_t index_count = 0;
    std::uint32_t item_begin = 0;
    std::uint32_t item_end = 0;
    IntRect bounds;
};

/** @brief A render target: the window canvas, or a LayerNode's texture. */
struct Target {
    LayerNode* layer = nullptr; // null for the canvas
    int depth = 0;              // layers nested above this one
    std::vector<DrawItem*> items;
    std::vector<Batch> batches;
    bool order_dirty = true;
    bool batches_dirty = true;
    bool bounds_dirty = true;
    GpuPool indices;

    rhi::BufferRef view_buffer;
    ViewData view{};
    bool view_dirty = true;
    bool view_used = false;
    rhi::BindGroupRef group0;
    std::uint64_t group0_generation = ~0ull;

    // Pixels: the canvas covers [0, extent); a layer covers `bounds` of its own pixel space.
    rhi::Extent2D extent;
    IntRect bounds;
    rhi::TextureRef color;
    rhi::TextureRef depth_stencil;
    rhi::RenderTargetRef render_target;
    rhi::BindGroupRef sample_group; // layers: the texture as a TextureMaterial bind group
    rhi::ResourceState color_state = rhi::ResourceState::Undefined;

    bool content_dirty = true; // layers: re-render the texture
    bool quad_dirty = true;    // layers: the quad in the parent target needs refreshing
    DamageRegion damage;       // canvas only
};

/** @brief A stencil clip's GPU data: its rectangle as 6 vertices, and a slot with its world transform. */
struct ClipItem {
    std::uint32_t slot = 0;
    std::uint32_t vertex_offset = 0;
    bool has_stencil = false;
    IntRect bounds;
    Target* target = nullptr;
};

/** @brief A sg::Texture's GPU copy. */
struct GpuTexture {
    std::weak_ptr<Texture> source;
    rhi::TextureRef texture;
    rhi::BindGroupRef smooth_group;
    rhi::BindGroupRef nearest_group;
};

/** @brief Per-frame-in-flight objects; the fence guards them all. */
struct Frame {
    rhi::CommandBufferRef command_buffer;
    rhi::FenceRef fence;
    rhi::SemaphoreRef image_ready;
    rhi::BufferRef staging;
    std::uint64_t staging_size = 0;
};

/** @brief One copy recorded in the upload pass. */
struct PendingCopy {
    std::uint64_t staging_offset = 0;
    std::uint64_t size = 0;
    rhi::BufferRef dst;
    std::uint64_t dst_offset = 0;
};

} // namespace

/** @cond INTERNAL */
// The renderer's state; observes the root and receives the update pass's findings. Hidden from the API docs.
struct Renderer::Impl final : NodeObserver, UpdateVisitor {
    RendererDesc desc;
    rhi::QueueRef queue;
    rhi::Format depth_format = rhi::Format::D32FloatS8Uint;
    RootNode* root = nullptr;
    FrameStats stats;
    std::uint64_t frame_counter = 0;

    // Size.
    rhi::Extent2D size;
    float dpr = 1.0f;
    bool resize_pending = true;
    rhi::Extent2D pending_size;
    float pending_dpr = 1.0f;
    bool force_all = true;

    // Fixed GPU objects.
    rhi::BindGroupLayoutRef group0_layout;
    rhi::BindGroupLayoutRef texture_layout;
    rhi::PipelineLayoutRef content_layout;
    rhi::PipelineLayoutRef composite_layout;
    rhi::ShaderRef node_vert, color_frag, texture_frag, composite_vert, composite_frag;
    rhi::RenderPassRef canvas_pass; // colour loaded
    rhi::RenderPassRef layer_pass;  // colour cleared
    rhi::RenderPassRef composite_pass;
    std::map<PipelineKey, rhi::PipelineRef> content_pipelines;
    rhi::PipelineRef clear_pipeline, stencil_reset_pipeline, stencil_increment_pipeline, composite_pipeline;
    rhi::SamplerRef smooth_sampler, nearest_sampler;

    // Pools.
    GpuPool vertices{rhi::BufferUsage::Vertex | rhi::BufferUsage::CopyDst, rhi::ResourceState::VertexBuffer,
                     "scenegraph vertices"};
    GpuPool slots{rhi::BufferUsage::Storage | rhi::BufferUsage::CopyDst, rhi::ResourceState::ShaderRead,
                  "scenegraph node slots"};
    RangeAllocator vertex_ranges;
    RangeAllocator slot_ranges;

    // Scene mirrors.
    Target canvas;
    std::unordered_map<const LayerNode*, std::unique_ptr<Target>> layers;
    std::unordered_map<const Node*, std::unique_ptr<DrawItem>> items;
    std::unordered_map<const ClipNode*, ClipItem> clips;
    std::unordered_map<std::uint64_t, GpuTexture> textures;
    std::vector<std::uint64_t> pending_texture_uploads;

    // Presentation.
    std::array<Frame, kFramesInFlight> frames;
    std::size_t frame_index = 0;
    rhi::SwapchainRef swapchain;
    bool swapchain_needs_resize = false;
    std::vector<rhi::RenderTargetRef> backbuffer_targets;
    std::vector<rhi::SemaphoreRef> render_finished;
    rhi::BindGroupRef composite_group;

    // ---- NodeObserver ----

    void nodeDirty() override { requestFrame(); }

    void nodeRemoved(Node& subtree) override {
        releaseSubtree(subtree);
        requestFrame();
    }

    void rootDestroyed() override {
        root = nullptr;
        resetScene();
    }

    // ---- UpdateVisitor ----

    void geometryUpdated(GeometryNode& node, DirtyFlags flags, bool) override {
        Target& target = targetOf(node.layer());
        auto [item, created] = itemFor(node, target);
        const Geometry& geometry = node.geometry();
        bool changed = created;

        if (created || hasAny(flags, DirtyFlags::Geometry | DirtyFlags::NodeAdded)) {
            changed |= writeGeometry(*item, geometry.vertices(), geometry.listIndices(), geometry.isLines());
        }
        if (created || hasAny(flags, DirtyFlags::MaterialType | DirtyFlags::NodeAdded)) {
            rhi::BindGroupRef group = textureGroup(node.material());
            if (group != item->texture_group) {
                item->texture_group = std::move(group);
                target.batches_dirty = true;
                changed = true;
            }
        }
        if (item->clip != node.clipContext()) {
            item->clip = node.clipContext();
            target.batches_dirty = true;
            changed = true;
        }
        changed |= writeSlot(item->slot, nodeData(node.worldTransform(), node.worldOpacity(), node.material().color));

        const bool visible = node.worldOpacity() > 0.0f;
        const IntRect bounds = visible ? deviceBounds(node.worldTransform(), geometry.bounds(), item->clip) : IntRect{};
        if (bounds != item->bounds || changed) {
            damage(target, item->bounds);
            damage(target, bounds);
            item->bounds = bounds;
            target.bounds_dirty = true;
        }
    }

    void clipUpdated(ClipNode& node) override {
        Target& target = targetOf(node.layer());
        ClipItem& clip = clips[&node];
        clip.target = &target;
        const IntRect bounds = IntRect::covering(node.context().scissor).intersected(targetPixels(target));
        // A clip change can reveal or hide pixels of children that did not change themselves.
        if (bounds != clip.bounds || node.usesStencil() || clip.has_stencil) {
            damage(target, clip.bounds);
            damage(target, bounds);
            clip.bounds = bounds;
        }
        if (node.usesStencil()) {
            if (!clip.has_stencil) {
                clip.slot = allocateSlot();
                clip.vertex_offset = allocateVertices(kClipVertexCount);
                clip.has_stencil = true;
            }
            const RectF r = node.rect();
            const std::array<PointF, kClipVertexCount> corners = {PointF{r.x, r.y},
                                                                  PointF{r.right(), r.y},
                                                                  PointF{r.right(), r.bottom()},
                                                                  PointF{r.right(), r.bottom()},
                                                                  PointF{r.x, r.bottom()},
                                                                  PointF{r.x, r.y}};
            std::array<GpuVertex, kClipVertexCount> data{};
            for (std::size_t i = 0; i < corners.size(); ++i) {
                data[i] = {corners[i].x, corners[i].y, 0.0f, 0.0f, 0xffffffffu, clip.slot};
            }
            vertices.write(std::uint64_t{clip.vertex_offset} * sizeof(GpuVertex), data.data(), sizeof(data));
            writeSlot(clip.slot, nodeData(node.worldTransform(), 1.0f, ColorF::white()));
        } else if (clip.has_stencil) {
            freeClipStencil(clip);
        }
    }

    void layerUpdated(LayerNode& node, int depth) override {
        Target& parent = targetOf(node.layer());
        auto [item, created] = itemFor(node, parent);
        Target& layer = layerTarget(node);
        layer.depth = depth + 1;
        if (created) {
            item->vertex_offset = allocateVertices(4);
            item->vertex_count = 4;
            item->indices = {0, 1, 2, 2, 3, 0};
            parent.batches_dirty = true;
        }
        if (item->clip != node.clipContext()) {
            item->clip = node.clipContext();
            parent.batches_dirty = true;
        }
        writeSlot(item->slot, nodeData(node.worldTransform(), node.worldOpacity(), ColorF::white()));
        layer.quad_dirty = true;
    }

    void structureChanged(LayerNode* layer) override {
        Target& target = targetOf(layer);
        target.order_dirty = true;
        if (target.layer != nullptr) {
            target.content_dirty = true;
        }
    }

    // ---- Scene bookkeeping ----

    void requestFrame() const {
        if (desc.request_frame) {
            desc.request_frame();
        }
    }

    Target& targetOf(LayerNode* layer) { return layer == nullptr ? canvas : layerTarget(*layer); }

    Target& layerTarget(LayerNode& layer) {
        std::unique_ptr<Target>& target = layers[&layer];
        if (!target) {
            target = std::make_unique<Target>();
            target->layer = &layer;
            target->indices = {rhi::BufferUsage::Index | rhi::BufferUsage::CopyDst, rhi::ResourceState::IndexBuffer,
                               "scenegraph layer indices"};
        }
        return *target;
    }

    /** @brief The pixels of a target, in the space its items' bounds use. */
    static IntRect targetPixels(const Target& target) {
        if (target.layer != nullptr) {
            return IntRect{-(1 << 24), -(1 << 24), 1 << 25, 1 << 25}; // layers grow to fit their content
        }
        return {0, 0, static_cast<std::int32_t>(target.extent.width), static_cast<std::int32_t>(target.extent.height)};
    }

    std::pair<DrawItem*, bool> itemFor(Node& node, Target& target) {
        std::unique_ptr<DrawItem>& item = items[&node];
        if (item) {
            return {item.get(), false};
        }
        item = std::make_unique<DrawItem>();
        item->node = &node;
        item->target = &target;
        item->slot = allocateSlot();
        target.order_dirty = true;
        return {item.get(), true};
    }

    std::uint32_t allocateSlot() {
        std::optional<std::uint32_t> slot = slot_ranges.allocate(1);
        if (!slot) {
            slot_ranges.grow(std::max<std::uint32_t>(slot_ranges.capacity() * 2, 256));
            slot = slot_ranges.allocate(1);
        }
        slots.reserve(std::uint64_t{slot_ranges.capacity()} * sizeof(NodeData));
        return *slot;
    }

    std::uint32_t allocateVertices(std::uint32_t count) {
        if (count == 0) {
            return 0;
        }
        std::optional<std::uint32_t> offset = vertex_ranges.allocate(count);
        while (!offset) {
            vertex_ranges.grow(std::max<std::uint32_t>(vertex_ranges.capacity() * 2, 4096));
            offset = vertex_ranges.allocate(count);
        }
        vertices.reserve(std::uint64_t{vertex_ranges.capacity()} * sizeof(GpuVertex));
        return *offset;
    }

    bool writeSlot(std::uint32_t slot, const NodeData& data) {
        return slots.write(std::uint64_t{slot} * sizeof(NodeData), &data, sizeof(data));
    }

    /** @brief Writes an item's vertices and indices; reallocates when the vertex count changes. @return Changed. */
    bool writeGeometry(DrawItem& item, const std::vector<Vertex>& source, std::vector<std::uint32_t> list_indices,
                       bool lines) {
        Target& target = *item.target;
        const auto count = static_cast<std::uint32_t>(source.size());
        bool changed = false;
        if (count != item.vertex_count) {
            vertex_ranges.free(item.vertex_offset, item.vertex_count);
            item.vertex_offset = allocateVertices(count);
            item.vertex_count = count;
            target.batches_dirty = true; // absolute indices change
            changed = true;
        }
        std::vector<GpuVertex> data(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            const Vertex& v = source[i];
            data[i] = {v.x, v.y, v.u, v.v, premultiply(v.rgba), item.slot};
        }
        if (count > 0) {
            changed |= vertices.write(std::uint64_t{item.vertex_offset} * sizeof(GpuVertex), data.data(),
                                      data.size() * sizeof(GpuVertex));
        }
        if (list_indices.size() != item.indices.size() || lines != item.lines) {
            target.batches_dirty = true;
            changed = true;
        } else if (list_indices != item.indices) {
            changed = true;
            if (!target.batches_dirty) {
                // Same count: patch this item's slice of the target's index buffer in place.
                std::vector<std::uint32_t> absolute(list_indices.size());
                for (std::size_t i = 0; i < absolute.size(); ++i) {
                    absolute[i] = item.vertex_offset + list_indices[i];
                }
                target.indices.write(std::uint64_t{item.index_pos} * 4, absolute.data(), absolute.size() * 4);
            }
        }
        item.indices = std::move(list_indices);
        item.lines = lines;
        return changed;
    }

    /** @brief Pixels covered by @p local under @p world, grown by one pixel (rasterization) and clipped. */
    static IntRect deviceBounds(const Transform2D& world, const RectF& local, const ClipContext* clip) {
        RectF rect = world.mapRect(local).adjusted(1.0f);
        if (clip != nullptr) {
            rect = rect.intersected(clip->scissor);
        }
        return IntRect::covering(rect);
    }

    void damage(Target& target, const IntRect& rect) {
        if (rect.isEmpty()) {
            return;
        }
        if (target.layer == nullptr) {
            target.damage.add(rect);
        } else {
            target.content_dirty = true;
        }
    }

    /** @brief Frees what the renderer holds for @p node and its subtree, damaging where they were drawn. */
    void releaseSubtree(Node& node) {
        for (const std::unique_ptr<Node>& child : node.children()) {
            releaseSubtree(*child);
        }
        if (auto it = items.find(&node); it != items.end()) {
            DrawItem& item = *it->second;
            damage(*item.target, item.bounds);
            item.target->order_dirty = true;
            vertex_ranges.free(item.vertex_offset, item.vertex_count);
            slot_ranges.free(item.slot, 1);
            items.erase(it);
        }
        if (node.type() == Node::Type::Clip) {
            if (auto it = clips.find(static_cast<const ClipNode*>(&node)); it != clips.end()) {
                damage(*it->second.target, it->second.bounds);
                freeClipStencil(it->second);
                clips.erase(it);
            }
        }
        if (node.type() == Node::Type::Layer) {
            layers.erase(static_cast<const LayerNode*>(&node));
        }
    }

    void freeClipStencil(ClipItem& clip) {
        if (clip.has_stencil) {
            slot_ranges.free(clip.slot, 1);
            vertex_ranges.free(clip.vertex_offset, kClipVertexCount);
            clip.has_stencil = false;
        }
    }

    /** @brief Forgets the whole scene; keeps GPU pools and pipelines. */
    void resetScene() {
        items.clear();
        layers.clear();
        clips.clear();
        canvas.items.clear();
        canvas.batches.clear();
        canvas.order_dirty = true;
        canvas.batches_dirty = true;
        vertex_ranges = RangeAllocator(vertex_ranges.capacity());
        slot_ranges = RangeAllocator(slot_ranges.capacity());
        reserveBackground();
        canvas.damage.addAll();
        force_all = true;
        requestFrame();
    }

    /** @brief Reserves slot 0 and vertices 0..2 for the background triangle, and writes them. */
    void reserveBackground() {
        if (slot_ranges.capacity() == 0) {
            slot_ranges.grow(256);
        }
        if (vertex_ranges.capacity() == 0) {
            vertex_ranges.grow(4096);
        }
        [[maybe_unused]] std::optional<std::uint32_t> slot = slot_ranges.allocate(1);
        [[maybe_unused]] std::optional<std::uint32_t> first = vertex_ranges.allocate(kBackgroundVertexCount);
        slots.reserve(std::uint64_t{slot_ranges.capacity()} * sizeof(NodeData));
        vertices.reserve(std::uint64_t{vertex_ranges.capacity()} * sizeof(GpuVertex));
        // Larger than any render target (16384 pixels), wherever its origin is.
        constexpr float kFar = 1.0e5f;
        const std::array<GpuVertex, kBackgroundVertexCount> triangle = {{
            {-kFar, -kFar, 0.0f, 0.0f, 0xffffffffu, kBackgroundSlot},
            {3.0f * kFar, -kFar, 0.0f, 0.0f, 0xffffffffu, kBackgroundSlot},
            {-kFar, 3.0f * kFar, 0.0f, 0.0f, 0xffffffffu, kBackgroundSlot},
        }};
        vertices.write(0, triangle.data(), sizeof(triangle));
        writeSlot(kBackgroundSlot, nodeData({}, 1.0f, desc.clear_color));
    }

    /** @brief The bind group for a material's texture (null without one); uploads the texture on first use. */
    rhi::BindGroupRef textureGroup(const Material& material) {
        if (!material.texture) {
            return nullptr;
        }
        GpuTexture& gpu = textures[material.texture->id()];
        if (!gpu.texture) {
            gpu.source = material.texture;
            const Texture& source = *material.texture;
            auto texture = desc.device->createTexture({.format = rhi::Format::RGBA8Unorm,
                                                       .extent = {source.width(), source.height()},
                                                       .usage = rhi::TextureUsage::Sampled | rhi::TextureUsage::CopyDst,
                                                       .label = desc.label + " texture"});
            if (!texture) {
                textures.erase(material.texture->id());
                return nullptr;
            }
            gpu.texture = *texture;
            auto view = desc.device->createTextureView({.texture = gpu.texture});
            if (!view) {
                textures.erase(material.texture->id());
                return nullptr;
            }
            gpu.smooth_group = makeTextureGroup(*view, smooth_sampler);
            gpu.nearest_group = makeTextureGroup(*view, nearest_sampler);
            pending_texture_uploads.push_back(material.texture->id());
        }
        return material.smooth ? gpu.smooth_group : gpu.nearest_group;
    }

    rhi::BindGroupRef makeTextureGroup(const rhi::TextureViewRef& view, const rhi::SamplerRef& sampler) const {
        auto group = desc.device->createBindGroup(
            {.layout = texture_layout,
             .entries = {{.binding = 0, .texture_view = view}, {.binding = 1, .sampler = sampler}}});
        return group ? *group : nullptr;
    }

    /** @brief Drops GPU copies of textures nobody references any more (in-flight frames keep their own refs). */
    void collectTextures() {
        std::erase_if(textures, [](const auto& entry) { return entry.second.source.expired(); });
    }

    // ---- Per-frame CPU work ----

    /** @brief Rebuilds a target's paint-ordered item list. */
    void rebuildOrder(Target& target) {
        target.items.clear();
        Node* top = target.layer != nullptr ? static_cast<Node*>(target.layer) : static_cast<Node*>(root);
        if (top != nullptr) {
            for (const std::unique_ptr<Node>& child : top->children()) {
                collect(*child, target);
            }
        }
        target.order_dirty = false;
        target.batches_dirty = true;
        target.bounds_dirty = true;
    }

    void collect(Node& node, Target& target) {
        if (node.type() == Node::Type::Geometry || node.type() == Node::Type::Layer) {
            if (auto it = items.find(&node); it != items.end()) {
                target.items.push_back(it->second.get());
            }
            if (node.type() == Node::Type::Layer) {
                return; // drawn into its own target
            }
        }
        for (const std::unique_ptr<Node>& child : node.children()) {
            collect(*child, target);
        }
    }

    /** @brief Splits a target's items into batches and rewrites its index buffer. */
    void rebuildBatches(Target& target) {
        target.batches.clear();
        std::size_t total = 0;
        for (const DrawItem* item : target.items) {
            total += item->indices.size();
        }
        std::vector<std::uint32_t> indices;
        indices.reserve(total);
        for (std::uint32_t i = 0; i < target.items.size(); ++i) {
            DrawItem& item = *target.items[i];
            const bool same = !target.batches.empty() && target.batches.back().lines == item.lines &&
                              target.batches.back().texture_group == item.texture_group &&
                              target.batches.back().clip == item.clip;
            if (!same) {
                target.batches.push_back({item.lines,
                                          item.texture_group,
                                          item.clip,
                                          static_cast<std::uint32_t>(indices.size()),
                                          0,
                                          i,
                                          i,
                                          {}});
            }
            item.index_pos = static_cast<std::uint32_t>(indices.size());
            for (std::uint32_t index : item.indices) {
                indices.push_back(item.vertex_offset + index);
            }
            Batch& batch = target.batches.back();
            batch.index_count += static_cast<std::uint32_t>(item.indices.size());
            batch.item_end = i + 1;
        }
        target.indices.mirror.assign(reinterpret_cast<const std::byte*>(indices.data()),
                                     reinterpret_cast<const std::byte*>(indices.data() + indices.size()));
        target.indices.markAllDirty();
        target.batches_dirty = false;
        target.bounds_dirty = true;
        stats.batches_rebuilt = true;
    }

    void updateBatchBounds(Target& target) {
        for (Batch& batch : target.batches) {
            batch.bounds = {};
            for (std::uint32_t i = batch.item_begin; i < batch.item_end; ++i) {
                batch.bounds = batch.bounds.united(target.items[i]->bounds);
            }
        }
        target.bounds_dirty = false;
    }

    /**
     * @brief Refreshes a layer's bounds, texture and quad, and damages the quad in its parent target when needed.
     */
    rhi::Result<void> updateLayer(Target& layer) {
        DrawItem* item = items.count(layer.layer) != 0 ? items[layer.layer].get() : nullptr;
        if (item == nullptr) {
            return {};
        }
        Target& parent = *item->target;
        if (layer.content_dirty) {
            IntRect bounds;
            for (const DrawItem* child : layer.items) {
                bounds = bounds.united(child->bounds);
            }
            if (bounds != layer.bounds) {
                layer.bounds = bounds;
                layer.view_dirty = true;
                layer.quad_dirty = true;
                const rhi::Extent2D needed{
                    static_cast<std::uint32_t>(
                        alignUp(static_cast<std::uint64_t>(std::max(bounds.width, 1)), kLayerTextureGranularity)),
                    static_cast<std::uint32_t>(
                        alignUp(static_cast<std::uint64_t>(std::max(bounds.height, 1)), kLayerTextureGranularity))};
                if (!bounds.isEmpty() &&
                    (!layer.color || layer.extent.width < needed.width || layer.extent.height < needed.height)) {
                    SG_TRY(createTargetTextures(layer, needed));
                    layer.sample_group =
                        makeTextureGroup(layer.render_target->getDesc().color_attachments[0], nearest_sampler);
                }
            }
        }
        if (layer.quad_dirty) {
            const bool visible = !layer.bounds.isEmpty() && layer.layer->worldOpacity() > 0.0f;
            // The quad, in the layer node's local units (layer pixels / pixel ratio).
            const float inv = 1.0f / dpr;
            const RectF local{layer.bounds.x * inv, layer.bounds.y * inv, layer.bounds.width * inv,
                              layer.bounds.height * inv};
            const float u1 = layer.extent.width > 0 ? static_cast<float>(layer.bounds.width) / layer.extent.width : 0;
            const float v1 =
                layer.extent.height > 0 ? static_cast<float>(layer.bounds.height) / layer.extent.height : 0;
            const std::array<GpuVertex, 4> quad = {{
                {local.x, local.y, 0.0f, 0.0f, 0xffffffffu, item->slot},
                {local.right(), local.y, u1, 0.0f, 0xffffffffu, item->slot},
                {local.right(), local.bottom(), u1, v1, 0xffffffffu, item->slot},
                {local.x, local.bottom(), 0.0f, v1, 0xffffffffu, item->slot},
            }};
            vertices.write(std::uint64_t{item->vertex_offset} * sizeof(GpuVertex), quad.data(), sizeof(quad));
            rhi::BindGroupRef group = visible ? layer.sample_group : nullptr;
            if (group != item->texture_group) {
                item->texture_group = std::move(group);
                parent.batches_dirty = true;
            }
            const IntRect bounds = visible ? deviceBounds(layer.layer->worldTransform(), local, item->clip) : IntRect{};
            damage(parent, item->bounds);
            damage(parent, bounds);
            item->bounds = bounds;
            parent.bounds_dirty = true;
            layer.quad_dirty = false;
        } else if (layer.content_dirty) {
            damage(parent, item->bounds); // same quad, new pixels
        }
        return {};
    }

    // ---- GPU objects ----

    rhi::Result<void> createFixedObjects() {
#if defined(MANGO_SCENEGRAPH_SHADERS)
        rhi::IDevice& device = *desc.device;
        SG_ASSIGN(group0_layout, device.createBindGroupLayout(
                                     {.entries = {{0, rhi::BindingType::UniformBuffer, rhi::ShaderStage::Vertex},
                                                  { 1,
                                                    rhi::BindingType::StorageBuffer,
                                                    rhi::ShaderStage::Vertex }},
                                      .label = desc.label + " view+nodes"}));
        SG_ASSIGN(texture_layout, device.createBindGroupLayout(
                                      {.entries = {{0, rhi::BindingType::SampledTexture, rhi::ShaderStage::Fragment},
                                                   { 1,
                                                     rhi::BindingType::Sampler,
                                                     rhi::ShaderStage::Fragment }},
                                       .label = desc.label + " texture"}));
        SG_ASSIGN(content_layout, device.createPipelineLayout({.bind_group_layouts = {
            group0_layout,
            texture_layout
        }}));
        SG_ASSIGN(composite_layout, device.createPipelineLayout({.bind_group_layouts = { texture_layout }}));

        const auto shader = [&](rhi::ShaderStage stage, std::span<const std::uint32_t> spirv, const char* name) {
            return device.createShader(
                {.stage = stage, .spirv = {spirv.begin(), spirv.end()}, .label = desc.label + " " + name});
        };
        SG_ASSIGN(node_vert, shader(rhi::ShaderStage::Vertex, node_vert_spv, "node.vert"));
        SG_ASSIGN(color_frag, shader(rhi::ShaderStage::Fragment, color_frag_spv, "color.frag"));
        SG_ASSIGN(texture_frag, shader(rhi::ShaderStage::Fragment, texture_frag_spv, "texture.frag"));
        SG_ASSIGN(composite_vert, shader(rhi::ShaderStage::Vertex, composite_vert_spv, "composite.vert"));
        SG_ASSIGN(composite_frag, shader(rhi::ShaderStage::Fragment, composite_frag_spv, "composite.frag"));

        // Depth is never used; the stencil buffer only holds rotated clips and starts cleared every pass.
        const auto pass = [&](rhi::LoadOp color_load, const char* name) {
            return device.createRenderPass({
                .color_attachments = {{
                    .format = kCanvasFormat,
                    .load_op = color_load
                }},
                .depth_stencil_attachment = rhi::AttachmentDesc{.format = depth_format,
                                                                .load_op = rhi::LoadOp::DontCare,
                                                                .store_op = rhi::StoreOp::DontCare,
                                                                .stencil_load_op = rhi::LoadOp::Clear},
                .label = desc.label + " " + name,
            });
        };
        SG_ASSIGN(canvas_pass, pass(rhi::LoadOp::Load, "canvas pass"));
        SG_ASSIGN(layer_pass, pass(rhi::LoadOp::Clear, "layer pass"));
        SG_ASSIGN(composite_pass, device.createRenderPass({.color_attachments =
                                                               {{.format = kSwapchainFormat,
                                                                 .load_op = rhi::LoadOp::DontCare }},
                                                           .label = desc.label + " composite pass"}));

        SG_ASSIGN(smooth_sampler, device.createSampler({.address_u = rhi::AddressMode::ClampToEdge,
                                                        .address_v = rhi::AddressMode::ClampToEdge,
                                                        .max_lod = 0.0f}));
        SG_ASSIGN(nearest_sampler, device.createSampler({.min_filter = rhi::Filter::Nearest,
                                                         .mag_filter = rhi::Filter::Nearest,
                                                         .mipmap_filter = rhi::Filter::Nearest,
                                                         .address_u = rhi::AddressMode::ClampToEdge,
                                                         .address_v = rhi::AddressMode::ClampToEdge,
                                                         .max_lod = 0.0f}));

        const rhi::BlendState premultiplied{.enabled = true,
                                            .src_color = rhi::BlendFactor::One,
                                            .dst_color = rhi::BlendFactor::OneMinusSrcAlpha,
                                            .src_alpha = rhi::BlendFactor::One,
                                            .dst_alpha = rhi::BlendFactor::OneMinusSrcAlpha};
        SG_ASSIGN(clear_pipeline, makePipeline(false, color_frag, {}, {}, "clear"));
        SG_ASSIGN(stencil_reset_pipeline, makePipeline(false, color_frag, {.write_mask = rhi::ColorWriteMask::None},
                                                       {.stencil_test = true,
                                                        .stencil =
                                                        {.compare = rhi::CompareOp::Always,
                                                         .pass_op = rhi::StencilOp::Zero }},
                                                       "stencil reset"));
        SG_ASSIGN(stencil_increment_pipeline, makePipeline(false, color_frag, {.write_mask = rhi::ColorWriteMask::None},
                                                           {.stencil_test = true,
                                                            .stencil =
                                                            {.compare = rhi::CompareOp::Equal,
                                                             .pass_op = rhi::StencilOp::IncrementClamp }},
                                                           "stencil clip"));
        for (bool lines : {false, true}) {
            for (bool textured : {false, true}) {
                for (bool stencil : {false, true}) {
                    rhi::DepthStencilState stencil_state{};
                    if (stencil) {
                        stencil_state = {.stencil_test = true, .stencil = {.compare = rhi::CompareOp::Equal }};
                    }
                    const PipelineKey key{lines, textured, stencil};
                    SG_ASSIGN(content_pipelines[key], makePipeline(lines, textured ? texture_frag : color_frag,
                                                                   premultiplied, stencil_state, "content"));
                }
            }
        }
        SG_ASSIGN(composite_pipeline, device.createGraphicsPipeline({.layout = composite_layout,
                                                                     .vertex_shader = composite_vert,
                                                                     .fragment_shader = composite_frag,
                                                                     .render_pass = composite_pass,
                                                                     .label = desc.label + " composite"}));

        for (Frame& frame : frames) {
            SG_ASSIGN(frame.command_buffer, device.createCommandBuffer({.label = desc.label + " frame"}));
            SG_ASSIGN(frame.fence, device.createFence({.signaled = true}));
            SG_ASSIGN(frame.image_ready, device.createSemaphore({}));
        }
        return {};
#else
        return rhi::Error{rhi::ErrorCode::Unsupported, "sg::Renderer: built without a GPU backend (no shaders)"};
#endif
    }

    rhi::Result<rhi::PipelineRef> makePipeline(bool lines, const rhi::ShaderRef& fragment, rhi::BlendState blend,
                                               rhi::DepthStencilState depth_stencil, const char* name) const {
        return desc.device->createGraphicsPipeline({
            .layout = content_layout,
            .vertex_shader = node_vert,
            .fragment_shader = fragment,
            .vertex_buffers = {{.stride = sizeof(GpuVertex),
                                .attributes = {{0, rhi::Format::RG32Float, offsetof(GpuVertex, x)},
                                               {1, rhi::Format::RG32Float, offsetof(GpuVertex, u)},
                                               {2, rhi::Format::RGBA8Unorm, offsetof(GpuVertex, rgba)},
                                               {3, rhi::Format::R32Uint, offsetof(GpuVertex, slot)}}}},
            .topology = lines ? rhi::PrimitiveTopology::LineList : rhi::PrimitiveTopology::TriangleList,
            .depth_stencil = depth_stencil,
            .blend = {blend},
            .render_pass = canvas_pass,
            .label = desc.label + " " + name,
        });
    }

    /** @brief (Re)creates a target's colour and depth-stencil textures at @p extent. */
    rhi::Result<void> createTargetTextures(Target& target, rhi::Extent2D extent) {
        rhi::IDevice& device = *desc.device;
        SG_ASSIGN(target.color, device.createTexture({.format = kCanvasFormat,
                                                      .extent = {extent.width, extent.height},
                                                      .usage = rhi::TextureUsage::RenderTarget |
                                                               rhi::TextureUsage::Sampled | rhi::TextureUsage::CopySrc,
                                                      .label = desc.label + (target.layer ? " layer" : " canvas")}));
        SG_ASSIGN(target.depth_stencil, device.createTexture({.format = depth_format,
                                                              .extent = {extent.width, extent.height},
                                                              .usage = rhi::TextureUsage::DepthStencil,
                                                              .label = desc.label + " stencil"}));
        rhi::TextureViewRef color_view;
        rhi::TextureViewRef depth_view;
        SG_ASSIGN(color_view, device.createTextureView({.texture = target.color}));
        SG_ASSIGN(depth_view, device.createTextureView({.texture = target.depth_stencil}));
        SG_ASSIGN(target.render_target, device.createRenderTarget({.render_pass = canvas_pass,
                                                                   .color_attachments = {color_view},
                                                                   .depth_stencil_attachment = depth_view,
                                                                   .extent = extent}));
        target.extent = extent;
        target.color_state = rhi::ResourceState::Undefined;
        target.view_dirty = true;
        return {};
    }

    /** @brief Makes sure a target has its view uniform buffer and an up-to-date bind group 0. */
    rhi::Result<void> prepareTargetBindings(Target& target) {
        if (!target.view_buffer) {
            SG_ASSIGN(target.view_buffer,
                      desc.device->createBuffer({.size = sizeof(ViewData),
                                                 .usage = rhi::BufferUsage::Uniform | rhi::BufferUsage::CopyDst,
                                                 .label = desc.label + " view"}));
            target.view_dirty = true;
            target.view_used = false;
            target.group0 = nullptr;
        }
        if (!target.group0 || target.group0_generation != slots.generation) {
            SG_ASSIGN(target.group0,
                      desc.device->createBindGroup({.layout = group0_layout,
                                                    .entries = {{.binding = 0, .buffer = target.view_buffer},
                                                                {.binding = 1, .buffer = slots.buffer}},
                                                    .label = desc.label + " view+nodes"}));
            target.group0_generation = slots.generation;
        }
        return {};
    }

    /** @brief Recreates a pool's GPU buffer when its mirror outgrew it. */
    rhi::Result<void> preparePool(GpuPool& pool) {
        if (pool.mirror.empty() || pool.buffer_size >= pool.mirror.size()) {
            return {};
        }
        // Grown in powers of two, so a slowly growing scene does not recreate the buffer every frame.
        const std::uint64_t buffer_size = growTo(pool.mirror.size(), 256);
        SG_ASSIGN(pool.buffer, desc.device->createBuffer({.size = buffer_size,
                                                          .usage = pool.usage,
                                                          .memory = rhi::MemoryLocation::GpuOnly,
                                                          .label = pool.label}));
        pool.buffer_size = buffer_size;
        pool.used = false;
        ++pool.generation;
        pool.markAllDirty();
        return {};
    }

    /** @brief Applies a pending resize: new canvas, swapchain size, full redraw. */
    rhi::Result<void> applyResize() {
        resize_pending = false;
        if (pending_dpr != dpr) {
            dpr = pending_dpr;
            force_all = true;
            for (auto& [node, layer] : layers) {
                layer->content_dirty = true;
            }
        }
        size = pending_size;
        if (size.width == 0 || size.height == 0) {
            return {};
        }
        SG_TRY(createTargetTextures(canvas, size));
        canvas.damage.setBounds(targetPixels(canvas));
        canvas.damage.addAll();
        composite_group = nullptr;
        if (desc.surface) {
            backbuffer_targets.clear();
            if (!swapchain) {
                SG_ASSIGN(swapchain, desc.device->createSwapchain({.surface = desc.surface,
                                                                   .extent = size,
                                                                   .format = kSwapchainFormat,
                                                                   .present_mode = desc.present_mode,
                                                                   .label = desc.label + " swapchain"}));
            } else {
                SG_TRY(swapchain->resize(size));
            }
            swapchain_needs_resize = false;
        }
        // Clips are clipped to the canvas, so their bounds depend on its size.
        force_all = true;
        return {};
    }

    // ---- Recording ----

    /** @brief Uploads dirty pool ranges, view uniforms and new textures through the frame's staging buffer. */
    rhi::Result<void> recordUploads(Frame& frame, rhi::ICommandBuffer& cmd) {
        SG_TRY(preparePool(vertices));
        SG_TRY(preparePool(slots));
        std::vector<Target*> targets = {&canvas};
        for (auto& [node, layer] : layers) {
            targets.push_back(layer.get());
        }
        for (Target* target : targets) {
            SG_TRY(preparePool(target->indices));
            SG_TRY(prepareTargetBindings(*target));
        }

        // Plan every copy, then size the staging buffer once.
        std::vector<PendingCopy> copies;
        std::vector<std::pair<const std::byte*, std::uint64_t>> sources;
        std::uint64_t staging_size = 0;
        rhi::BarrierBatch before;
        rhi::BarrierBatch after;
        const auto planPool = [&](GpuPool& pool) {
            pool.coalesce();
            if (pool.dirty.empty() || !pool.buffer) {
                pool.dirty.clear();
                return;
            }
            for (const auto& [begin, end] : pool.dirty) {
                copies.push_back({staging_size, end - begin, pool.buffer, begin});
                sources.emplace_back(pool.mirror.data() + begin, end - begin);
                staging_size = alignUp(staging_size + (end - begin), 16);
            }
            pool.dirty.clear();
            const rhi::ResourceState state = pool.used ? pool.read_state : rhi::ResourceState::Undefined;
            before.buffers.push_back({.buffer = pool.buffer, .before = state, .after = rhi::ResourceState::CopyDst});
            after.buffers.push_back(
                {.buffer = pool.buffer, .before = rhi::ResourceState::CopyDst, .after = pool.read_state});
            pool.used = true;
        };
        planPool(vertices);
        planPool(slots);
        std::vector<ViewData> views;
        views.reserve(targets.size());
        for (Target* target : targets) {
            planPool(target->indices);
            if (target->view_dirty) {
                const float width = target->layer ? static_cast<float>(target->extent.width) : size.width;
                const float height = target->layer ? static_cast<float>(target->extent.height) : size.height;
                views.push_back({static_cast<float>(target->layer ? target->bounds.x : 0),
                                 static_cast<float>(target->layer ? target->bounds.y : 0), 2.0f / width,
                                 2.0f / height});
                copies.push_back({staging_size, sizeof(ViewData), target->view_buffer, 0});
                sources.emplace_back(reinterpret_cast<const std::byte*>(&views.back()), sizeof(ViewData));
                staging_size = alignUp(staging_size + sizeof(ViewData), 16);
                const rhi::ResourceState state =
                    target->view_used ? rhi::ResourceState::UniformBuffer : rhi::ResourceState::Undefined;
                before.buffers.push_back(
                    {.buffer = target->view_buffer, .before = state, .after = rhi::ResourceState::CopyDst});
                after.buffers.push_back({.buffer = target->view_buffer,
                                         .before = rhi::ResourceState::CopyDst,
                                         .after = rhi::ResourceState::UniformBuffer});
                target->view_dirty = false;
                target->view_used = true;
            }
        }
        // Textures: premultiplied on the way into the staging buffer.
        struct TextureCopy {
            std::uint64_t staging_offset;
            rhi::TextureRef texture;
            std::uint32_t width, height;
            std::shared_ptr<Texture> source;
        };
        std::vector<TextureCopy> texture_copies;
        for (std::uint64_t id : pending_texture_uploads) {
            auto it = textures.find(id);
            if (it == textures.end()) {
                continue;
            }
            std::shared_ptr<Texture> source = it->second.source.lock();
            if (!source) {
                continue;
            }
            staging_size = alignUp(staging_size, 256);
            texture_copies.push_back({staging_size, it->second.texture, source->width(), source->height(), source});
            staging_size += source->pixels().size();
            before.textures.push_back({.texture = it->second.texture,
                                       .before = rhi::ResourceState::Undefined,
                                       .after = rhi::ResourceState::CopyDst});
            after.textures.push_back({.texture = it->second.texture,
                                      .before = rhi::ResourceState::CopyDst,
                                      .after = rhi::ResourceState::ShaderRead});
        }
        pending_texture_uploads.clear();
        if (staging_size == 0) {
            return {};
        }

        if (frame.staging_size < staging_size) {
            frame.staging_size = growTo(staging_size, 64 * 1024);
            SG_ASSIGN(frame.staging, desc.device->createBuffer({.size = frame.staging_size,
                                                                .usage = rhi::BufferUsage::CopySrc,
                                                                .memory = rhi::MemoryLocation::CpuToGpu,
                                                                .label = desc.label + " staging"}));
        }
        rhi::MapResult mapped = frame.staging->map({});
        if (!mapped) {
            return mapped.error();
        }
        for (std::size_t i = 0; i < copies.size(); ++i) {
            std::memcpy(mapped->data() + copies[i].staging_offset, sources[i].first, sources[i].second);
        }
        for (const TextureCopy& copy : texture_copies) {
            const std::vector<std::uint8_t>& pixels = copy.source->pixels();
            std::byte* dst = mapped->data() + copy.staging_offset;
            for (std::size_t p = 0; p + 3 < pixels.size(); p += 4) {
                std::uint32_t rgba = 0;
                std::memcpy(&rgba, &pixels[p], 4);
                rgba = premultiply(rgba);
                std::memcpy(dst + p, &rgba, 4);
            }
        }
        frame.staging->unmap();
        stats.bytes_uploaded += staging_size;

        cmd.barrier(std::move(before));
        {
            rhi::CopyEncoderRef copy = cmd.beginCopy();
            for (const PendingCopy& c : copies) {
                copy->copyBuffer({.src = frame.staging,
                                  .src_offset = c.staging_offset,
                                  .dst = c.dst,
                                  .dst_offset = c.dst_offset,
                                  .size = c.size});
            }
            for (const TextureCopy& c : texture_copies) {
                copy->copyBufferTexture({.buffer = {.buffer = frame.staging, .offset = c.staging_offset},
                                         .texture = {.texture = c.texture},
                                         .extent = {c.width, c.height}});
            }
            copy->end();
        }
        cmd.barrier(std::move(after));
        return {};
    }

    /** @brief Draws a target's batches that intersect @p rects (in the target's item pixel space). */
    void recordTarget(rhi::ICommandBuffer& cmd, Target& target, const std::vector<IntRect>& rects) {
        cmd.barrier(
            {.textures = {
                 {.texture = target.color, .before = target.color_state, .after = rhi::ResourceState::RenderTarget},
                 {.texture = target.depth_stencil,
                  .before = rhi::ResourceState::Undefined,
                  .after = rhi::ResourceState::DepthStencilWrite}}});
        const bool is_layer = target.layer != nullptr;
        rhi::RenderEncoderRef pass = cmd.beginRender({.render_pass = is_layer ? layer_pass : canvas_pass,
                                                      .target = target.render_target,
                                                      .clear_colors = {{0.0f, 0.0f, 0.0f, 0.0f}}});
        const std::int32_t origin_x = is_layer ? target.bounds.x : 0;
        const std::int32_t origin_y = is_layer ? target.bounds.y : 0;
        pass->setVertexBuffers({.buffers = {{vertices.buffer}}});
        if (target.indices.buffer) {
            pass->setIndexBuffer({.buffer = target.indices.buffer, .format = rhi::IndexFormat::Uint32});
        }

        rhi::PipelineRef current;
        const auto use = [&](const rhi::PipelineRef& pipeline) {
            if (pipeline != current) {
                pass->setPipeline(pipeline);
                pass->setBindGroup({.index = 0, .bind_group = target.group0});
                current = pipeline;
            }
        };
        const auto scissor = [&](const IntRect& r) {
            pass->setScissor({r.x - origin_x, r.y - origin_y, static_cast<std::uint32_t>(std::max(r.width, 0)),
                              static_cast<std::uint32_t>(std::max(r.height, 0))});
        };

        for (const IntRect& rect : rects) {
            scissor(rect);
            if (!is_layer) {
                use(clear_pipeline);
                pass->draw({.vertex_count = kBackgroundVertexCount});
                ++stats.draw_calls;
            }
            const ClipContext* stencil_ready = nullptr;
            IntRect stencil_rect;
            rhi::BindGroupRef bound_texture;
            for (const Batch& batch : target.batches) {
                if (batch.index_count == 0 || !batch.bounds.intersects(rect)) {
                    continue;
                }
                IntRect area = rect;
                if (batch.clip != nullptr) {
                    area = area.intersected(IntRect::covering(batch.clip->scissor));
                    if (area.isEmpty()) {
                        continue;
                    }
                }
                const bool stencil = batch.clip != nullptr && !batch.clip->stencil_clips.empty();
                if (stencil && (stencil_ready != batch.clip || stencil_rect != area)) {
                    // Rebuild the clip mask in this area: zero it, then each clip increments where the previous ones
                    // passed, so the pixels inside all N clips end at N.
                    scissor(area);
                    use(stencil_reset_pipeline);
                    pass->draw({.vertex_count = kBackgroundVertexCount});
                    use(stencil_increment_pipeline);
                    const auto& chain = batch.clip->stencil_clips;
                    for (std::uint32_t i = 0; i < chain.size(); ++i) {
                        auto it = clips.find(chain[i]);
                        if (it == clips.end() || !it->second.has_stencil) {
                            continue;
                        }
                        pass->setStencilReference(i);
                        pass->draw({.vertex_count = kClipVertexCount, .first_vertex = it->second.vertex_offset});
                    }
                    stats.draw_calls += 1 + static_cast<std::uint32_t>(chain.size());
                    stencil_ready = batch.clip;
                    stencil_rect = area;
                }
                scissor(area);
                const rhi::PipelineRef& pipeline =
                    content_pipelines[{batch.lines, batch.texture_group != nullptr, stencil}];
                const bool pipeline_changed = pipeline != current;
                use(pipeline);
                if (batch.texture_group && (pipeline_changed || bound_texture != batch.texture_group)) {
                    pass->setBindGroup({.index = 1, .bind_group = batch.texture_group});
                    bound_texture = batch.texture_group;
                }
                if (stencil) {
                    pass->setStencilReference(static_cast<std::uint32_t>(batch.clip->stencil_clips.size()));
                }
                pass->drawIndexed({.index_count = batch.index_count, .first_index = batch.first_index});
                ++stats.batch_draws;
                ++stats.draw_calls;
            }
        }
        pass->end();
        cmd.barrier({.textures = {{.texture = target.color,
                                   .before = rhi::ResourceState::RenderTarget,
                                   .after = rhi::ResourceState::ShaderRead}}});
        target.color_state = rhi::ResourceState::ShaderRead;
    }

    rhi::Result<rhi::RenderTargetRef> backbufferTarget(std::uint32_t index, const rhi::TextureRef& texture) {
        if (backbuffer_targets.size() <= index) {
            backbuffer_targets.resize(index + 1);
        }
        if (!backbuffer_targets[index]) {
            rhi::TextureViewRef view;
            SG_ASSIGN(view, desc.device->createTextureView({.texture = texture}));
            const rhi::TextureDesc texture_desc = texture->getDesc();
            SG_ASSIGN(
                backbuffer_targets[index],
                desc.device->createRenderTarget({.render_pass = composite_pass,
                                                 .color_attachments = {view},
                                                 .extent = {texture_desc.extent.width, texture_desc.extent.height}}));
        }
        return backbuffer_targets[index];
    }

    rhi::Result<rhi::SemaphoreRef> renderFinished(std::uint32_t index) {
        if (render_finished.size() <= index) {
            render_finished.resize(index + 1);
        }
        if (!render_finished[index]) {
            SG_ASSIGN(render_finished[index], desc.device->createSemaphore({}));
        }
        return render_finished[index];
    }

    /** @brief Waits until no frame is in flight. */
    rhi::Result<void> waitIdle() {
        for (Frame& frame : frames) {
            SG_TRY(frame.fence->wait(rhi::kWaitForever));
        }
        return {};
    }

    // ---- The frame ----

    rhi::Result<FrameStats> render() {
        stats = {};
        stats.frame = ++frame_counter;
        if (resize_pending || swapchain_needs_resize) {
            if (swapchain_needs_resize && !resize_pending) {
                pending_size = size;
                pending_dpr = dpr;
            }
            SG_TRY(applyResize());
        }
        if (size.width == 0 || size.height == 0) {
            stats.skipped = true;
            return stats;
        }

        if (root != nullptr && (force_all || root->dirtyFlags() != DirtyFlags::None)) {
            stats.nodes_visited = UpdatePass::run(*root, dpr, force_all, this).nodes_visited;
        }
        force_all = false;

        if (canvas.order_dirty) {
            rebuildOrder(canvas);
        }
        std::vector<Target*> layer_list;
        for (auto& [node, layer] : layers) {
            if (layer->order_dirty) {
                rebuildOrder(*layer);
            }
            layer_list.push_back(layer.get());
        }
        // Deepest first: a nested layer's quad is part of its parent's content.
        std::ranges::sort(layer_list, [](const Target* a, const Target* b) { return a->depth > b->depth; });
        std::vector<Target*> layers_to_render;
        for (Target* layer : layer_list) {
            if (layer->batches_dirty) {
                rebuildBatches(*layer);
            }
            const bool render_content = layer->content_dirty;
            SG_TRY(updateLayer(*layer));
            if (render_content && layer->color && !layer->bounds.isEmpty()) {
                layers_to_render.push_back(layer);
            }
        }
        if (canvas.batches_dirty) {
            rebuildBatches(canvas);
        }
        for (Target* target : layer_list) {
            if (target->bounds_dirty) {
                updateBatchBounds(*target);
            }
            stats.batches += static_cast<std::uint32_t>(target->batches.size());
        }
        if (canvas.bounds_dirty) {
            updateBatchBounds(canvas);
        }
        stats.batches += static_cast<std::uint32_t>(canvas.batches.size());

        if (canvas.damage.isEmpty() && layers_to_render.empty()) {
            stats.skipped = true; // pending uploads (e.g. invisible nodes) wait for a frame that draws
            return stats;
        }

        Frame& frame = frames[frame_index];
        SG_TRY(frame.fence->wait(rhi::kWaitForever));
        rhi::AcquireResult acquired;
        if (swapchain) {
            acquired = swapchain->acquire({.signal_semaphore = frame.image_ready});
            if (acquired.status == rhi::AcquireStatus::OutOfDate) {
                swapchain_needs_resize = true;
                requestFrame();
                stats.skipped = true;
                return stats;
            }
            if (acquired.status != rhi::AcquireStatus::Success && acquired.status != rhi::AcquireStatus::Suboptimal) {
                return rhi::Error{rhi::ErrorCode::SurfaceLost, "sg::Renderer: swapchain acquire failed"};
            }
            if (acquired.status == rhi::AcquireStatus::Suboptimal) {
                swapchain_needs_resize = true; // this image is still presented; resize before the next one
                requestFrame();
            }
        }
        collectTextures();

        SG_TRY(frame.fence->reset());
        rhi::ICommandBuffer& cmd = *frame.command_buffer;
        SG_TRY(cmd.begin());
        SG_TRY(recordUploads(frame, cmd));
        for (Target* layer : layers_to_render) {
            recordTarget(cmd, *layer, {layer->bounds});
            layer->content_dirty = false;
            ++stats.layers_rendered;
        }
        stats.full_redraw = canvas.damage.isFull();
        stats.damage_rects = static_cast<std::uint32_t>(canvas.damage.rects().size());
        stats.damage_area = canvas.damage.area();
        if (!canvas.damage.isEmpty()) {
            recordTarget(cmd, canvas, canvas.damage.rects());
        }
        canvas.damage.clear();

        if (!swapchain) {
            SG_TRY(cmd.end());
            SG_TRY(queue->submit({.command_buffers = {frame.command_buffer}, .fence = frame.fence}));
            frame_index = (frame_index + 1) % kFramesInFlight;
            return stats;
        }

        if (!composite_group) {
            composite_group = makeTextureGroup(canvas.render_target->getDesc().color_attachments[0], nearest_sampler);
        }
        rhi::RenderTargetRef backbuffer;
        SG_ASSIGN(backbuffer, backbufferTarget(acquired.image_index, acquired.texture));
        cmd.barrier({.textures = {{.texture = acquired.texture,
                                   .before = rhi::ResourceState::Undefined,
                                   .after = rhi::ResourceState::RenderTarget}}});
        {
            rhi::RenderEncoderRef pass = cmd.beginRender({.render_pass = composite_pass, .target = backbuffer});
            pass->setPipeline(composite_pipeline);
            pass->setBindGroup({.index = 0, .bind_group = composite_group});
            pass->draw({.vertex_count = 3});
            pass->end();
            ++stats.draw_calls;
        }
        cmd.barrier({.textures = {{.texture = acquired.texture,
                                   .before = rhi::ResourceState::RenderTarget,
                                   .after = rhi::ResourceState::Present}}});
        SG_TRY(cmd.end());
        rhi::SemaphoreRef finished;
        SG_ASSIGN(finished, renderFinished(acquired.image_index));
        SG_TRY(queue->submit({.command_buffers = {frame.command_buffer},
                              .waits = {{frame.image_ready, 0, rhi::PipelineStage::RenderTargetOutput}},
                              .signals = {{finished, 0}},
                              .fence = frame.fence}));
        rhi::Result<void> presented = queue->present(
            {.swapchain = swapchain, .image_index = acquired.image_index, .wait_semaphores = {finished}});
        if (!presented) {
            if (presented.error().code != rhi::ErrorCode::OutOfDate) {
                return presented.error();
            }
            swapchain_needs_resize = true;
            canvas.damage.addAll(); // presented nothing; the canvas itself is fine, but redraw to be safe
            requestFrame();
        }
        frame_index = (frame_index + 1) % kFramesInFlight;
        return stats;
    }

    rhi::Result<std::vector<std::uint8_t>> readPixels() {
        if (!canvas.color) {
            return rhi::Error{rhi::ErrorCode::InvalidArgument, "sg::Renderer::readPixels: nothing rendered yet"};
        }
        SG_TRY(waitIdle());
        const rhi::Extent2D extent = canvas.extent;
        const std::uint64_t bytes = std::uint64_t{extent.width} * extent.height * 4;
        rhi::BufferRef readback;
        SG_ASSIGN(readback, desc.device->createBuffer({.size = bytes,
                                                       .usage = rhi::BufferUsage::CopyDst,
                                                       .memory = rhi::MemoryLocation::GpuToCpu,
                                                       .label = desc.label + " readback"}));
        rhi::CommandBufferRef cmd;
        SG_ASSIGN(cmd, desc.device->createCommandBuffer({.label = desc.label + " readback"}));
        SG_TRY(cmd->begin());
        cmd->barrier(
            {.textures = {
                 {.texture = canvas.color, .before = canvas.color_state, .after = rhi::ResourceState::CopySrc}}});
        {
            rhi::CopyEncoderRef copy = cmd->beginCopy();
            copy->copyBufferTexture({.direction = rhi::CopyDirection::TextureToBuffer,
                                     .buffer = {.buffer = readback},
                                     .texture = {.texture = canvas.color},
                                     .extent = {extent.width, extent.height}});
            copy->end();
        }
        const rhi::ResourceState restore =
            canvas.color_state == rhi::ResourceState::Undefined ? rhi::ResourceState::ShaderRead : canvas.color_state;
        cmd->barrier(
            {.textures = {{.texture = canvas.color, .before = rhi::ResourceState::CopySrc, .after = restore}}});
        canvas.color_state = restore;
        SG_TRY(cmd->end());
        rhi::FenceRef fence;
        SG_ASSIGN(fence, desc.device->createFence({}));
        SG_TRY(queue->submit({.command_buffers = {cmd}, .fence = fence}));
        SG_TRY(fence->wait(rhi::kWaitForever));
        rhi::MapResult mapped = readback->map({});
        if (!mapped) {
            return mapped.error();
        }
        std::vector<std::uint8_t> pixels(bytes);
        std::memcpy(pixels.data(), mapped->data(), bytes);
        readback->unmap();
        return pixels;
    }
};

/** @endcond */

rhi::Result<std::unique_ptr<Renderer>> Renderer::create(RendererDesc desc) {
    if (!desc.device) {
        return rhi::Error{rhi::ErrorCode::InvalidArgument, "sg::Renderer::create: no device"};
    }
    auto impl = std::make_unique<Impl>();
    impl->desc = std::move(desc);
    if (impl->desc.label.empty()) {
        impl->desc.label = "scenegraph";
    }
    impl->queue = impl->desc.device->getQueue(rhi::QueueType::Graphics);
    impl->canvas.indices = {rhi::BufferUsage::Index | rhi::BufferUsage::CopyDst, rhi::ResourceState::IndexBuffer,
                            "scenegraph canvas indices"};
    // Not every GPU has a D32 + S8 format (e.g. some Vulkan drivers only offer D24 + S8).
    if (!impl->desc.device->createTexture(
            {.format = rhi::Format::D32FloatS8Uint, .extent = {1, 1}, .usage = rhi::TextureUsage::DepthStencil})) {
        impl->depth_format = rhi::Format::D24UnormS8Uint;
    }
    SG_TRY(impl->createFixedObjects());
    impl->pending_size = impl->desc.size;
    impl->pending_dpr = impl->desc.device_pixel_ratio > 0.0f ? impl->desc.device_pixel_ratio : 1.0f;
    impl->dpr = impl->pending_dpr;
    impl->resize_pending = true;
    impl->reserveBackground();
    return std::unique_ptr<Renderer>(new Renderer(std::move(impl)));
}

Renderer::Renderer(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {
}

Renderer::~Renderer() {
    if (m_impl->root != nullptr) {
        m_impl->root->setObserver(nullptr);
    }
}

void Renderer::setRootNode(RootNode* root) {
    if (m_impl->root == root) {
        return;
    }
    if (m_impl->root != nullptr) {
        m_impl->root->setObserver(nullptr);
    }
    m_impl->root = root;
    if (root != nullptr) {
        root->setObserver(m_impl.get());
    }
    m_impl->resetScene();
}

RootNode* Renderer::rootNode() const {
    return m_impl->root;
}

void Renderer::resize(rhi::Extent2D size, float device_pixel_ratio) {
    const float dpr = device_pixel_ratio > 0.0f ? device_pixel_ratio : 1.0f;
    if (!m_impl->resize_pending && size == m_impl->size && dpr == m_impl->dpr) {
        return;
    }
    m_impl->pending_size = size;
    m_impl->pending_dpr = dpr;
    m_impl->resize_pending = true;
    m_impl->requestFrame();
}

rhi::Extent2D Renderer::size() const {
    return m_impl->resize_pending ? m_impl->pending_size : m_impl->size;
}

void Renderer::setClearColor(ColorF color) {
    m_impl->desc.clear_color = color;
    m_impl->writeSlot(kBackgroundSlot, nodeData({}, 1.0f, color));
    m_impl->canvas.damage.addAll();
    m_impl->requestFrame();
}

bool Renderer::needsRender() const {
    const Impl& impl = *m_impl;
    if (impl.resize_pending || impl.swapchain_needs_resize || impl.force_all || !impl.canvas.damage.isEmpty()) {
        return true;
    }
    return impl.root != nullptr && impl.root->dirtyFlags() != DirtyFlags::None;
}

rhi::Result<FrameStats> Renderer::render() {
    return m_impl->render();
}

const FrameStats& Renderer::lastFrameStats() const {
    return m_impl->stats;
}

rhi::Result<std::vector<std::uint8_t>> Renderer::readPixels() {
    return m_impl->readPixels();
}

} // namespace sg
