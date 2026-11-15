/**
 * @file
 * @brief scenegraph_tests: headless checks of the scene graph and its renderer, run by CTest.
 *
 * Runs every check, prints `[ OK ]` / `[FAIL]` per check, and exits non-zero if any failed. The CPU checks (math,
 * tree operations, dirty tracking, geometry, damage, allocation) always run; the renderer checks run offscreen once
 * per available RHI backend, with validation on, read the canvas back and compare pixels. Any validation warning
 * fails the run.
 */

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "ClipNode.h"
#include "DamageRegion.h"
#include "GeometryNode.h"
#include "LayerNode.h"
#include "OpacityNode.h"
#include "RangeAllocator.h"
#include "Renderer.h"
#include "TransformNode.h"
#include "UpdatePass.h"

namespace {

int g_failures = 0; // failed checks so far

/// Prints the outcome of one check and counts failures.
void check(bool condition, const std::string& what) {
    std::printf("%s %s\n", condition ? "[ OK ]" : "[FAIL]", what.c_str());
    if (!condition) {
        ++g_failures;
    }
}

bool near(float a, float b, float epsilon = 1.0e-4f) {
    return std::abs(a - b) <= epsilon;
}

// ---------------------------------------------------------------------------------------------------------------
// CPU checks
// ---------------------------------------------------------------------------------------------------------------

void testMath() {
    using sg::RectF;
    using sg::Transform2D;
    const Transform2D t = Transform2D::translation(10, 20) * Transform2D::scaling(2, 3);
    const sg::PointF p = t.map({1, 1});
    check(near(p.x, 12) && near(p.y, 23), "Transform2D composes right to left");
    const auto inverse = t.inverted();
    const sg::PointF back = inverse ? inverse->map(p) : sg::PointF{};
    check(inverse && near(back.x, 1) && near(back.y, 1), "Transform2D inverts");
    check(!Transform2D::scaling(0, 1).inverted(), "a singular transform has no inverse");
    const RectF rotated = Transform2D::rotation(3.14159265f / 2).mapRect({0, 0, 10, 20});
    check(near(rotated.x, -20, 1e-3f) && near(rotated.width, 20, 1e-3f) && near(rotated.height, 10, 1e-3f),
          "mapRect gives the bounding box of a rotated rectangle");
    check(!Transform2D::rotation(0.3f).isAxisAligned() && Transform2D::scaling(-1, 2).isAxisAligned(),
          "isAxisAligned: rotations no, flips and scales yes");
    check((RectF{0, 0, 10, 10}.intersected({5, 5, 10, 10}) == RectF{5, 5, 5, 5}) &&
              RectF{0, 0, 1, 1}.intersected({2, 2, 1, 1}).isEmpty(),
          "RectF intersection");
    check(RectF{0, 0, 1, 1}.united({4, 4, 1, 1}) == RectF{0, 0, 5, 5} &&
              RectF{}.united({1, 1, 1, 1}) == RectF{1, 1, 1, 1},
          "RectF union ignores empty rectangles");
    check(sg::IntRect::covering({0.5f, 0.5f, 1.0f, 1.0f}) == sg::IntRect{0, 0, 2, 2},
          "IntRect::covering rounds outwards");
}

void testTree() {
    sg::RootNode root;
    auto* a = root.emplaceChild<sg::Node>();
    auto* b = root.emplaceChild<sg::Node>();
    auto* c = static_cast<sg::Node*>(root.insertChildBefore(std::make_unique<sg::Node>(), b));
    check(root.children().size() == 3 && root.children()[1].get() == c && c->parent() == &root,
          "insertChildBefore places the child before the given sibling");
    auto* grandchild = a->emplaceChild<sg::Node>();
    check(grandchild->root() == &root, "root() finds the RootNode");
    std::unique_ptr<sg::Node> removed = root.removeChild(a);
    check(removed.get() == a && a->parent() == nullptr && grandchild->root() == nullptr && root.children().size() == 2,
          "removeChild hands back ownership and detaches the subtree");
    check(root.removeChild(a) == nullptr, "removeChild of a non-child returns null");
}

/// Counts observer notifications and what the update pass reports.
struct Recorder : sg::NodeObserver, sg::UpdateVisitor {
    int dirty_notifications = 0;
    int removed_notifications = 0;
    int geometry_updates = 0;
    int layer_updates = 0;
    int structure_changes = 0;
    void nodeDirty() override { ++dirty_notifications; }
    void nodeRemoved(sg::Node&) override { ++removed_notifications; }
    void rootDestroyed() override {}
    void geometryUpdated(sg::GeometryNode&, sg::DirtyFlags, bool) override { ++geometry_updates; }
    void layerUpdated(sg::LayerNode&, int) override { ++layer_updates; }
    void structureChanged(sg::LayerNode*) override { ++structure_changes; }
    void reset() { *this = Recorder{}; }
};

void testDirtyTracking() {
    sg::RootNode root;
    Recorder recorder;
    root.setObserver(&recorder);
    // root -> left (transform) -> 10 rectangles; root -> right (transform) -> chain of 3 nodes -> leaf rectangle.
    auto* left = root.emplaceChild<sg::TransformNode>();
    for (int i = 0; i < 10; ++i) {
        left->emplaceChild<sg::RectangleNode>(sg::RectF{0, 0, 1, 1});
    }
    auto* right = root.emplaceChild<sg::TransformNode>();
    sg::Node* chain = right;
    for (int i = 0; i < 3; ++i) {
        chain = chain->emplaceChild<sg::Node>();
    }
    auto* leaf = chain->emplaceChild<sg::RectangleNode>(sg::RectF{0, 0, 1, 1});
    check(recorder.dirty_notifications > 0, "adding nodes notifies the observer");

    sg::UpdateStats first = sg::UpdatePass::run(root, 1.0f, false, &recorder);
    check(first.nodes_visited == 17 && recorder.geometry_updates == 11, "the first update visits every new node");
    check(root.dirtyFlags() == sg::DirtyFlags::None && leaf->dirtyFlags() == sg::DirtyFlags::None,
          "the update pass clears the flags");

    recorder.reset();
    leaf->setColor({1, 0, 0, 1});
    check(recorder.dirty_notifications == 1, "a change notifies once");
    leaf->setColor({0, 1, 0, 1});
    check(recorder.dirty_notifications == 1, "a second change on the same dirty path does not notify again");
    sg::UpdateStats leaf_only = sg::UpdatePass::run(root, 1.0f, false, &recorder);
    check(leaf_only.nodes_visited == 6 && recorder.geometry_updates == 1,
          "a leaf change visits only the path to it (root, right, 3 chain nodes, leaf)");

    recorder.reset();
    sg::UpdateStats clean = sg::UpdatePass::run(root, 1.0f, false, &recorder);
    check(clean.nodes_visited == 1 && recorder.geometry_updates == 0, "an unchanged tree visits only the root");

    recorder.reset();
    left->setMatrix(sg::Transform2D::translation(5, 0));
    sg::UpdatePass::run(root, 1.0f, false, &recorder);
    check(recorder.geometry_updates == 10, "moving a TransformNode recomputes exactly its subtree");
    check(near(left->children()[0]->worldTransform().tx, 5), "children inherit the new world transform");

    recorder.reset();
    left->setMatrix(sg::Transform2D::translation(5, 0));
    check(recorder.dirty_notifications == 0 && left->dirtyFlags() == sg::DirtyFlags::None,
          "setting an unchanged value marks nothing");

    // Layers: moving the layer updates its quad only, not the content.
    auto* layer = root.emplaceChild<sg::TransformNode>()->emplaceChild<sg::LayerNode>();
    for (int i = 0; i < 5; ++i) {
        layer->emplaceChild<sg::RectangleNode>(sg::RectF{0, 0, 1, 1});
    }
    sg::UpdatePass::run(root, 1.0f, false, &recorder);
    recorder.reset();
    static_cast<sg::TransformNode*>(layer->parent())->setMatrix(sg::Transform2D::translation(3, 3));
    sg::UpdatePass::run(root, 1.0f, false, &recorder);
    check(recorder.layer_updates == 1 && recorder.geometry_updates == 0,
          "moving a LayerNode updates the layer, not its content");
    check(near(layer->children()[0]->worldTransform().tx, 0) && layer->children()[0]->layer() == layer,
          "layer content is in the layer's own coordinates");

    recorder.reset();
    std::unique_ptr<sg::Node> taken = root.removeChild(right);
    check(recorder.removed_notifications == 1 && recorder.dirty_notifications == 1, "removal notifies the observer");
    sg::UpdatePass::run(root, 1.0f, false, &recorder);
    check(recorder.structure_changes == 1, "removal reports a structure change");
    root.setObserver(nullptr);
}

void testGeometry() {
    const sg::Geometry rect = sg::Geometry::rectangle({1, 2, 3, 4});
    check(rect.vertices().size() == 4 && rect.listIndices().size() == 6 && rect.bounds() == sg::RectF{1, 2, 3, 4},
          "rectangle geometry: 4 vertices, 2 triangles, bounds");
    const sg::Geometry strip(sg::DrawMode::TriangleStrip, std::vector<sg::Vertex>(5));
    check(strip.listIndices().size() == 9, "a 5-vertex strip becomes 3 triangles");
    const std::array<sg::PointF, 5> pentagon = {{{0, 0}, {2, 0}, {3, 1}, {1, 3}, {-1, 1}}};
    check(sg::Geometry::convexPolygon(pentagon).listIndices().size() == 9, "a pentagon fans into 3 triangles");
    const std::array<sg::PointF, 3> line = {{{0, 0}, {10, 0}, {10, 10}}};
    const sg::Geometry thick = sg::Geometry::polyline(line, 2.0f);
    check(thick.vertices().size() == 8 && near(thick.bounds().y, -1) && near(thick.bounds().right(), 11),
          "polyline: one quad per segment, half the width on each side");
    const sg::Geometry lines(sg::DrawMode::LineStrip, std::vector<sg::Vertex>(4));
    check(lines.isLines() && lines.listIndices().size() == 6, "a line strip becomes a line list");
    const sg::Geometry bad(sg::DrawMode::Triangles, std::vector<sg::Vertex>(3), {0, 1, 7});
    check(bad.listIndices().empty(), "primitives with out-of-range indices are dropped");
}

void testDamageRegion() {
    sg::DamageRegion damage(4, 0.5);
    damage.setBounds({0, 0, 100, 100});
    damage.add({10, 10, 5, 5});
    damage.add({12, 12, 5, 5});
    check(damage.rects().size() == 1 && damage.rects()[0] == sg::IntRect{10, 10, 7, 7}, "overlapping damage merges");
    damage.add({50, 50, 2, 2});
    check(damage.rects().size() == 2, "separate damage stays separate");
    damage.add({-10, -10, 15, 15});
    check(damage.rects().size() == 3 && damage.rects().back() == sg::IntRect{0, 0, 5, 5},
          "damage is clipped to the bounds");
    damage.add({80, 0, 1, 1});
    damage.add({0, 80, 1, 1});
    check(damage.rects().size() == 1, "too many rectangles collapse into their bounding box");
    damage.clear();
    damage.add({0, 0, 80, 80});
    check(damage.isFull() && damage.area() == 10000, "damage above the threshold becomes a full redraw");
}

void testRangeAllocator() {
    sg::RangeAllocator ranges(10);
    const auto a = ranges.allocate(4);
    const auto b = ranges.allocate(4);
    check(a == 0u && b == 4u && !ranges.allocate(4) && ranges.used() == 8, "first-fit allocation; full pool fails");
    ranges.free(*a, 4);
    check(ranges.allocate(3) == 0u, "freed ranges are reused");
    ranges.free(0, 3);
    ranges.free(*b, 4);
    check(ranges.allocate(10) == 0u, "freed neighbours merge back into one range");
    ranges.grow(20);
    check(ranges.allocate(10) == 10u && ranges.capacity() == 20, "grow adds free space at the end");
}

// ---------------------------------------------------------------------------------------------------------------
// Renderer checks (offscreen, per backend)
// ---------------------------------------------------------------------------------------------------------------

constexpr std::uint32_t kSize = 64;

/// One RGBA8 pixel of a read-back canvas.
struct Pixel {
    int r, g, b, a;
};

Pixel pixelAt(const std::vector<std::uint8_t>& pixels, std::uint32_t width, int x, int y) {
    const std::size_t i = (static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)) * 4;
    return {pixels[i], pixels[i + 1], pixels[i + 2], pixels[i + 3]};
}

bool pixelIs(const std::vector<std::uint8_t>& pixels, std::uint32_t width, int x, int y, Pixel expected,
             int tolerance = 2) {
    const Pixel p = pixelAt(pixels, width, x, y);
    return std::abs(p.r - expected.r) <= tolerance && std::abs(p.g - expected.g) <= tolerance &&
           std::abs(p.b - expected.b) <= tolerance && std::abs(p.a - expected.a) <= tolerance;
}

constexpr Pixel kWhite{255, 255, 255, 255};
constexpr Pixel kRed{255, 0, 0, 255};
constexpr Pixel kGreen{0, 255, 0, 255};
constexpr Pixel kBlue{0, 0, 255, 255};

/// An offscreen renderer of kSize x kSize (or @p size) with a white background.
std::unique_ptr<sg::Renderer> makeRenderer(const rhi::DeviceRef& device, std::uint32_t size = kSize) {
    auto renderer = sg::Renderer::create({.device = device, .size = {size, size}, .label = "test"});
    if (!renderer) {
        check(false, "renderer created: " + renderer.error().message);
        return nullptr;
    }
    return std::move(renderer).value();
}

/// Renders and reads back; empty on failure.
std::vector<std::uint8_t> renderAndRead(sg::Renderer& renderer) {
    auto stats = renderer.render();
    if (!stats) {
        check(false, "render: " + stats.error().message);
        return {};
    }
    auto pixels = renderer.readPixels();
    if (!pixels) {
        check(false, "readPixels: " + pixels.error().message);
        return {};
    }
    return std::move(pixels).value();
}

void testBasicDrawing(const rhi::DeviceRef& device) {
    auto renderer = makeRenderer(device);
    if (!renderer) {
        return;
    }
    sg::RootNode root;
    root.emplaceChild<sg::RectangleNode>(sg::RectF{8, 8, 16, 16}, sg::ColorF{1, 0, 0, 1});
    auto* moved = root.emplaceChild<sg::TransformNode>(sg::Transform2D::translation(32, 0));
    auto* faded = moved->emplaceChild<sg::OpacityNode>(0.5f);
    faded->emplaceChild<sg::RectangleNode>(sg::RectF{0, 0, 16, 16}, sg::ColorF{0, 0, 1, 1});
    const std::array<sg::PointF, 3> strip = {{{2, 60}, {60, 60}, {60, 40}}};
    root.emplaceChild<sg::GeometryNode>(sg::Geometry::polyline(strip, 2.0f), sg::Material::flatColor({0, 1, 0, 1}));
    renderer->setRootNode(&root);

    const std::vector<std::uint8_t> pixels = renderAndRead(*renderer);
    if (pixels.empty()) {
        return;
    }
    check(pixelIs(pixels, kSize, 15, 15, kRed) && pixelIs(pixels, kSize, 7, 15, kWhite) &&
              pixelIs(pixels, kSize, 24, 15, kWhite),
          "a red rectangle covers exactly its pixels");
    check(pixelIs(pixels, kSize, 40, 8, {128, 128, 255, 255}),
          "transform and opacity: 50% blue over white, premultiplied");
    check(pixelIs(pixels, kSize, 30, 60, kGreen) && pixelIs(pixels, kSize, 60, 50, kGreen),
          "a polyline draws its segments");
    check(renderer->lastFrameStats().full_redraw, "the first frame is a full redraw");
}

void testTexturesAndOrder(const rhi::DeviceRef& device) {
    auto renderer = makeRenderer(device);
    if (!renderer) {
        return;
    }
    auto texture = sg::Texture::fromRgba8(2, 2, {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 128});
    sg::RootNode root;
    root.emplaceChild<sg::ImageNode>(sg::RectF{0, 0, 32, 32}, texture, false);
    // Paint order across batches: red, then the texture, then green on top of both.
    root.emplaceChild<sg::RectangleNode>(sg::RectF{40, 0, 16, 16}, sg::ColorF{1, 0, 0, 1});
    root.emplaceChild<sg::ImageNode>(sg::RectF{44, 4, 16, 16}, texture, false);
    root.emplaceChild<sg::RectangleNode>(sg::RectF{48, 8, 16, 16}, sg::ColorF{0, 1, 0, 1});
    renderer->setRootNode(&root);
    const std::vector<std::uint8_t> pixels = renderAndRead(*renderer);
    if (pixels.empty()) {
        return;
    }
    check(pixelIs(pixels, kSize, 4, 4, kRed) && pixelIs(pixels, kSize, 20, 4, kGreen) &&
              pixelIs(pixels, kSize, 4, 20, kBlue),
          "a nearest-filtered texture shows its texels");
    check(pixelIs(pixels, kSize, 20, 20, {255, 255, 255, 255}), "half-transparent white texel over white stays white");
    check(pixelIs(pixels, kSize, 50, 10, kGreen) && pixelIs(pixels, kSize, 45, 5, kRed),
          "batches keep paint order across materials");
    check(renderer->lastFrameStats().batches == 4, "texture / colour / texture / colour make 4 batches");
}

void testIncrementalUpdates(const rhi::DeviceRef& device) {
    auto renderer = makeRenderer(device, 256);
    if (!renderer) {
        return;
    }
    sg::RootNode root;
    // Four clipped quadrants of 50 small rectangles each, and one mover in the first quadrant.
    std::vector<sg::RectangleNode*> rects;
    std::array<sg::ClipNode*, 4> quadrants{};
    for (int q = 0; q < 4; ++q) {
        const float qx = (q % 2) * 128.0f;
        const float qy = (q / 2) * 128.0f;
        quadrants[q] = root.emplaceChild<sg::ClipNode>(sg::RectF{qx, qy, 128, 128});
        for (int i = 0; i < 50; ++i) {
            rects.push_back(quadrants[q]->emplaceChild<sg::RectangleNode>(
                sg::RectF{qx + (i % 10) * 12.0f + 2, qy + (i / 10) * 24.0f + 2, 8, 8},
                sg::ColorF{0.1f * (i % 10), 0.2f * q, 0.5f, 1.0f}));
        }
    }
    auto* mover = quadrants[0]->emplaceChild<sg::TransformNode>();
    mover->emplaceChild<sg::RectangleNode>(sg::RectF{0, 0, 10, 10}, sg::ColorF{1, 0, 0, 1});
    renderer->setRootNode(&root);
    auto first = renderer->render();
    check(first && first->batches == 4 && first->batch_draws == 4, "4 clipped quadrants make 4 batches");

    auto clean = renderer->render();
    check(clean && clean->skipped && clean->draw_calls == 0 && clean->bytes_uploaded == 0 && clean->nodes_visited == 0,
          "an unchanged scene skips the frame: no draws, no uploads, no traversal");
    check(!renderer->needsRender(), "needsRender() is false when nothing changed");

    mover->setMatrix(sg::Transform2D::translation(5, 3));
    check(renderer->needsRender(), "needsRender() is true after a change");
    auto moved = renderer->render();
    check(moved && !moved->skipped && moved->bytes_uploaded == 48 && !moved->batches_rebuilt,
          "moving a node uploads one 48-byte slot and keeps the batches");
    check(moved && moved->damage_rects <= 2 && moved->damage_area <= 20 * 20 && !moved->full_redraw,
          "moving a 10x10 rectangle damages only around it");
    check(moved && moved->batch_draws == 1, "only the batch under the damage is drawn");
    check(moved && moved->nodes_visited <= 4, "the update pass visits only the mover's path");

    rects[0]->setColor({0, 0, 0, 1});
    auto recolored = renderer->render();
    check(recolored && recolored->bytes_uploaded == 48 && !recolored->batches_rebuilt,
          "recolouring a rectangle uploads one slot");

    rects[1]->setRect({4, 4, 6, 6});
    auto reshaped = renderer->render();
    check(reshaped && reshaped->bytes_uploaded == 4 * 24 && !reshaped->batches_rebuilt,
          "resizing a rectangle rewrites its 4 vertices in place");

    // Random edits, each rendered incrementally; the result must equal a full redraw of the same scene.
    std::mt19937 random(1234);
    std::uniform_real_distribution<float> offset(-20.0f, 20.0f);
    for (int step = 0; step < 30; ++step) {
        switch (step % 4) {
        case 0:
            mover->setMatrix(sg::Transform2D::translation(offset(random) + 40, offset(random) + 40));
            break;
        case 1:
            rects[random() % rects.size()]->setRect({offset(random) + 60, offset(random) + 60, 8, 8});
            break;
        case 2:
            rects[random() % rects.size()]->setColor({0.3f, 0.7f, 0.2f, 0.6f});
            break;
        case 3:
            quadrants[random() % 4]->setRect({offset(random) + 64, offset(random) + 64, 128, 128});
            break;
        }
        if (!renderer->render()) {
            check(false, "incremental render");
            return;
        }
    }
    auto incremental = renderer->readPixels();
    renderer->setClearColor(sg::ColorF::white()); // damages everything
    auto full = renderer->render();
    auto reference = renderer->readPixels();
    check(full && full->full_redraw && incremental && reference && *incremental == *reference,
          "30 random incremental frames equal a full redraw, pixel for pixel");

    // Removal reveals the background.
    std::unique_ptr<sg::Node> removed = root.removeChild(quadrants[3]);
    const std::vector<std::uint8_t> after_removal = renderAndRead(*renderer);
    check(!after_removal.empty() && pixelIs(after_removal, 256, 130, 130, kWhite) &&
              renderer->lastFrameStats().batches_rebuilt,
          "removing a subtree redraws the background where it was");
}

void testBatching(const rhi::DeviceRef& device) {
    auto renderer = makeRenderer(device, 256);
    if (!renderer) {
        return;
    }
    sg::RootNode root;
    for (int i = 0; i < 1000; ++i) {
        root.emplaceChild<sg::RectangleNode>(
            sg::RectF{static_cast<float>(i % 40) * 6, static_cast<float>(i / 40) * 9, 4, 4},
            sg::ColorF{0.2f, 0.4f, 0.8f, 1.0f});
    }
    renderer->setRootNode(&root);
    auto stats = renderer->render();
    check(stats && stats->batches == 1 && stats->batch_draws == 1, "1000 rectangles draw in one call");
    auto texture = sg::Texture::fromRgba8(1, 1, {255, 0, 0, 255});
    root.insertChildBefore(std::make_unique<sg::ImageNode>(sg::RectF{0, 0, 8, 8}, texture), root.children()[500].get());
    auto split = renderer->render();
    check(split && split->batches == 3 && split->batches_rebuilt, "an image in the middle splits the batch in three");
}

void testClipping(const rhi::DeviceRef& device) {
    auto renderer = makeRenderer(device);
    if (!renderer) {
        return;
    }
    sg::RootNode root;
    // Axis-aligned clip on the left half.
    auto* clip = root.emplaceChild<sg::ClipNode>(sg::RectF{4, 4, 20, 20});
    clip->emplaceChild<sg::RectangleNode>(sg::RectF{0, 0, 64, 32}, sg::ColorF{1, 0, 0, 1});
    // A 45-degree rotated clip (a diamond) around (44, 44), nested in a second rotated clip.
    auto* rotate = root.emplaceChild<sg::TransformNode>(sg::Transform2D::translation(44, 44) *
                                                        sg::Transform2D::rotation(3.14159265f / 4));
    auto* diamond = rotate->emplaceChild<sg::ClipNode>(sg::RectF{-10, -10, 20, 20});
    diamond->emplaceChild<sg::RectangleNode>(sg::RectF{-30, -30, 60, 60}, sg::ColorF{0, 0, 1, 1});
    renderer->setRootNode(&root);
    std::vector<std::uint8_t> pixels = renderAndRead(*renderer);
    if (pixels.empty()) {
        return;
    }
    check(pixelIs(pixels, kSize, 10, 10, kRed) && pixelIs(pixels, kSize, 30, 10, kWhite) &&
              pixelIs(pixels, kSize, 10, 28, kWhite),
          "an axis-aligned clip cuts its children");
    check(pixelIs(pixels, kSize, 44, 44, kBlue) && pixelIs(pixels, kSize, 44, 32, kBlue),
          "a rotated clip keeps the inside of the diamond");
    check(pixelIs(pixels, kSize, 33, 33, kWhite) && pixelIs(pixels, kSize, 55, 55, kWhite),
          "a rotated clip removes the corners of its bounding box (stencil)");

    // Nest an axis-aligned clip inside the diamond, then a second rotated one: the intersection remains.
    std::unique_ptr<sg::Node> content = diamond->removeChild(diamond->children()[0].get());
    auto* inner_rotate = diamond->emplaceChild<sg::TransformNode>(sg::Transform2D::rotation(-3.14159265f / 4));
    auto* inner = inner_rotate->emplaceChild<sg::ClipNode>(sg::RectF{0, -20, 20, 40});
    inner->emplaceChild<sg::RectangleNode>(sg::RectF{-30, -30, 60, 60}, sg::ColorF{0, 0, 1, 1});
    pixels = renderAndRead(*renderer);
    if (pixels.empty()) {
        return;
    }
    check(pixelIs(pixels, kSize, 48, 44, kBlue) && pixelIs(pixels, kSize, 40, 44, kWhite) &&
              pixelIs(pixels, kSize, 60, 44, kWhite) && pixelIs(pixels, kSize, 55, 44, kBlue),
          "nested rotated clips keep only their intersection");
}

void testLayers(const rhi::DeviceRef& device) {
    // Reference: the same content without a layer.
    auto build = [](sg::Node& parent) {
        for (int i = 0; i < 500; ++i) {
            parent.emplaceChild<sg::RectangleNode>(
                sg::RectF{static_cast<float>(i % 25) * 1.0f, static_cast<float>(i / 25) * 1.0f, 2, 2},
                sg::ColorF{(i % 7) / 7.0f, (i % 3) / 3.0f, 0.5f, 1.0f});
        }
    };
    auto reference_renderer = makeRenderer(device);
    auto renderer = makeRenderer(device);
    if (!renderer || !reference_renderer) {
        return;
    }
    sg::RootNode reference_root;
    auto* reference_move = reference_root.emplaceChild<sg::TransformNode>(sg::Transform2D::translation(10, 10));
    build(*reference_move);
    reference_renderer->setRootNode(&reference_root);

    sg::RootNode root;
    auto* move = root.emplaceChild<sg::TransformNode>(sg::Transform2D::translation(10, 10));
    auto* layer = move->emplaceChild<sg::LayerNode>();
    build(*layer);
    renderer->setRootNode(&root);
    auto first = renderer->render();
    check(first && first->layers_rendered == 1, "a new layer renders its texture once");

    move->setMatrix(sg::Transform2D::translation(30, 20));
    reference_move->setMatrix(sg::Transform2D::translation(30, 20));
    auto moved = renderer->render();
    check(moved && moved->layers_rendered == 0 && moved->nodes_visited <= 3 && moved->bytes_uploaded == 48,
          "moving a layer re-renders nothing: one slot upload");
    auto pixels = renderer->readPixels();
    auto reference = renderAndRead(*reference_renderer);
    bool same = pixels && !reference.empty() && pixels->size() == reference.size();
    for (std::size_t i = 0; same && i < reference.size(); ++i) {
        same = std::abs(static_cast<int>((*pixels)[i]) - static_cast<int>(reference[i])) <= 2;
    }
    check(same, "a moved layer looks like the same content drawn directly");

    static_cast<sg::RectangleNode*>(layer->children()[0].get())->setColor({0, 0, 0, 1});
    auto changed = renderer->render();
    check(changed && changed->layers_rendered == 1, "changing layer content re-renders the layer");
}

void testRendererBackend(rhi::Backend backend) {
    std::printf("-- renderer on %s\n", std::string(rhi::toString(backend)).c_str());
    int problems = 0;
    rhi::RHIFactory factory;
    auto instance = factory.createInstance(
        backend,
        {.application_name = "scenegraph_tests",
         .enable_validation = true,
         .debug_callback = [&problems](const rhi::DebugMessage& message) {
             if (message.severity == rhi::DebugSeverity::Warning || message.severity == rhi::DebugSeverity::Error) {
                 ++problems;
                 std::printf("[debug] %.*s\n", static_cast<int>(message.message.size()), message.message.data());
             }
         }});
    if (!instance) {
        check(false, "instance created: " + instance.error().message);
        return;
    }
    rhi::AdapterList adapters = (*instance)->enumerateAdapters();
    if (adapters.empty()) {
        check(false, "an adapter is available");
        return;
    }
    auto device = adapters[0]->createDevice({});
    if (!device) {
        check(false, "device created: " + device.error().message);
        return;
    }
    testBasicDrawing(*device);
    testTexturesAndOrder(*device);
    testIncrementalUpdates(*device);
    testBatching(*device);
    testClipping(*device);
    testLayers(*device);
    check(problems == 0, "no validation warnings or errors");
}

} // namespace

int main() {
    testMath();
    testTree();
    testDirtyTracking();
    testGeometry();
    testDamageRegion();
    testRangeAllocator();
    rhi::RHIFactory factory;
    for (rhi::Backend backend : factory.availableBackends()) {
        testRendererBackend(backend);
    }
    std::printf("%d check(s) failed\n", g_failures);
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
