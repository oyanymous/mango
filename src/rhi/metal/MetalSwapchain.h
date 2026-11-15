#pragma once

#import <QuartzCore/CAMetalLayer.h>

#include <memory>
#include <vector>

#include "ISurface.h"
#include "ISwapchain.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalTexture;

/// A `CAMetalLayer` to present to, from a MetalLayerHandle.
class MetalSurface final : public ISurface {
public:
    /**
     * Wraps the layer of a window.
     *
     * @param desc A MetalLayerHandle with a non-null `CAMetalLayer*`; the layer must outlive the surface.
     * @return The surface, or InvalidArgument for another handle type or a null layer.
     */
    static Result<SurfaceRef> create(SurfaceDesc desc);

    /// Wraps a layer. Use create().
    MetalSurface(SurfaceDesc desc, CAMetalLayer* layer);

    SurfaceDesc getDesc() const override;

    CAMetalLayer* layer() const { return m_layer; }
private:
    SurfaceDesc m_desc;
    CAMetalLayer* m_layer = nil;
};

/**
 * Backbuffers from a `CAMetalLayer`'s drawables.
 *
 * Metal hands out a new drawable per frame rather than a fixed set of images, so the swapchain keeps one MetalTexture
 * per drawable slot (`maximumDrawableCount`, 2 or 3) and points the next slot at the drawable's texture on each
 * acquire(). Views and render targets of a backbuffer therefore stay valid across frames, until resize().
 *
 * acquire() blocks in `nextDrawable` until a drawable is free (at most 1 s, then AcquireStatus::Timeout), so the
 * image is ready when it returns and the acquire semaphore is signaled from the CPU. MetalQueue::present presents the
 * drawable after the waited semaphores. PresentMode::Immediate turns off `displaySyncEnabled`; Fifo and Mailbox keep
 * vsync (Core Animation always replaces a queued frame, like Mailbox). Configure and use the layer on the main thread.
 */
class MetalSwapchain final : public ISwapchain {
public:
    /**
     * Configures the surface's layer for this device and creates the backbuffer slots.
     *
     * @return The swapchain, InvalidArgument for a null surface, or Unsupported for a format a `CAMetalLayer` cannot
     *         use (BGRA8Unorm, BGRA8Srgb and RGBA16Float can).
     */
    static Result<SwapchainRef> create(std::shared_ptr<MetalDeviceContext> context, SwapchainDesc desc);

    /// Wraps a configured layer. Use create().
    MetalSwapchain(std::shared_ptr<MetalDeviceContext> context, SwapchainDesc desc, CAMetalLayer* layer);

    AcquireResult acquire(AcquireDesc desc) override;
    Result<void> resize(Extent2D extent) override;

    /**
     * Takes the drawable acquired for @p image_index, for presentation.
     *
     * @return The drawable, or nil if that image was not acquired (or was already presented).
     */
    id<CAMetalDrawable> takeDrawable(std::uint32_t image_index);
private:
    // Creates one backbuffer slot per drawable at m_desc.extent.
    void createBackbuffers();

    SwapchainDesc m_desc;
    CAMetalLayer* m_layer = nil;
    std::vector<std::shared_ptr<MetalTexture>> m_backbuffers;
    std::vector<id<CAMetalDrawable>> m_drawables; // acquired and not yet presented, per slot
    std::uint32_t m_next_image = 0;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
