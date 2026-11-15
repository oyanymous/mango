#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ColorF.h"
#include "RHI.h"
#include "SceneNode.h"

namespace sg {

/// Describes a Renderer to create with Renderer::create.
struct RendererDesc {
    rhi::DeviceRef device;   /**< The device to render with; retained. */
    rhi::SurfaceRef surface; /**< Window surface to present to; null renders offscreen only (see readPixels()). */
    rhi::Extent2D size;      /**< Size in pixels; 0 x 0 (e.g. minimized) renders nothing until resize(). */
    float device_pixel_ratio = 1.0f;                        /**< Pixels per logical unit (2 on Retina displays). */
    ColorF clear_color = ColorF::white();                   /**< Background behind the scene. */
    rhi::PresentMode present_mode = rhi::PresentMode::Fifo; /**< Presentation mode of the swapchain. */
    /// Called when the renderer needs render() to run: the scene changed, the size changed, or a frame could not be
    /// presented. Typically `Application::requestFrame`. May be empty.
    std::function<void()> request_frame;
    std::string label; /**< Debug name prefix of the GPU objects. */
};

/// Counters describing one render() call.
struct FrameStats {
    std::uint64_t frame = 0;           /**< Number of render() calls so far, this one included. */
    bool skipped = false;              /**< Nothing changed: no GPU work was recorded, submitted or presented. */
    bool full_redraw = false;          /**< The whole canvas was redrawn (resize, or damage above the threshold). */
    std::uint32_t nodes_visited = 0;   /**< Nodes the update pass looked at (1 when only the root was checked). */
    std::uint32_t batches = 0;         /**< Batches in the canvas and every layer. */
    bool batches_rebuilt = false;      /**< Some render target's batches were rebuilt (paint order or materials). */
    std::uint32_t batch_draws = 0;     /**< Indexed draws of batches, in the canvas and layers. */
    std::uint32_t draw_calls = 0;      /**< Every draw: batches plus background clears, stencil clips and composite. */
    std::uint32_t layers_rendered = 0; /**< Layers whose texture was re-rendered. */
    std::uint64_t bytes_uploaded = 0;  /**< Bytes copied to the GPU (vertices, indices, node slots, textures). */
    std::uint32_t damage_rects = 0;    /**< Canvas rectangles redrawn. */
    std::int64_t damage_area = 0;      /**< Canvas pixels redrawn. */
};

/**
 * Draws one scene graph into a window (or an offscreen canvas), doing as little GPU work as possible.
 *
 * The scene is drawn into a persistent canvas texture that keeps its contents between frames; each frame redraws
 * only the canvas rectangles whose content changed (the damage), then copies the canvas onto the swapchain image.
 * When nothing changed, render() returns at once without touching the GPU. Geometry is grouped into batches
 * (consecutive nodes in paint order with the same pipeline, texture and clip are drawn with one indexed draw), and
 * only changed vertices, indices and 48-byte per-node slots are uploaded.
 *
 * The renderer does not own the scene: setRootNode() attaches it as the root's observer, and destroying the root
 * detaches it. Single-threaded: call everything, and change the scene, on one thread.
 *
 * @code
 * auto renderer = sg::Renderer::create({.device = device, .surface = surface, .size = {800, 600},
 *                                       .request_frame = [] { Application::requestFrame(); }});
 * sg::RootNode root;
 * root.emplaceChild<sg::RectangleNode>(sg::RectF{10, 10, 100, 50}, sg::ColorF{1, 0, 0, 1});
 * (*renderer)->setRootNode(&root);
 * Application::setFrameCallback([&] { (*renderer)->render(); }, FrameMode::OnDemand);
 * @endcode
 */
class Renderer {
public:
    /**
     * Creates the renderer, its canvas and (with a surface) its swapchain.
     *
     * @return The renderer, or an error: no device, a backend without the scene graph shaders (Unsupported), or a
     *         failed GPU object creation.
     */
    static rhi::Result<std::unique_ptr<Renderer>> create(RendererDesc desc);
    /// Detaches from the root. In-flight GPU work is kept alive by the RHI.
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /**
     * Sets the scene to draw (or null for none); the next render() redraws everything.
     *
     * @param root The root; must outlive the renderer or be destroyed (or replaced here) first.
     */
    void setRootNode(RootNode* root);
    /// The scene, or null.
    [[nodiscard]] RootNode* rootNode() const;

    /**
     * Changes the size (in pixels) and pixel ratio, e.g. from a window's resize callback.
     *
     * Takes effect in the next render(), which redraws everything. A 0 x 0 size renders nothing.
     */
    void resize(rhi::Extent2D size, float device_pixel_ratio);
    /// The current size in pixels (after the pending resize, if any).
    [[nodiscard]] rhi::Extent2D size() const;

    /// Changes the background colour (redraws everything).
    void setClearColor(ColorF color);

    /// Whether render() has anything to do.
    [[nodiscard]] bool needsRender() const;

    /**
     * Updates the GPU copy of the scene and draws the damaged parts; presents when there is a surface.
     *
     * Waits for the GPU only when every frame in flight (two) is still busy. Returns immediately, with
     * FrameStats::skipped, when nothing changed.
     *
     * @return The frame's counters, or an error from the RHI (device lost, out of memory, ...).
     */
    rhi::Result<FrameStats> render();
    /// The counters of the last render().
    [[nodiscard]] const FrameStats& lastFrameStats() const;

    /**
     * Reads the canvas back: `width * height * 4` bytes of RGBA8, premultiplied alpha, top row first.
     *
     * Waits until the GPU is idle. Shows what the last render() drew (before any pending changes).
     */
    rhi::Result<std::vector<std::uint8_t>> readPixels();
private:
    struct Impl;
    explicit Renderer(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;
};

} // namespace sg
