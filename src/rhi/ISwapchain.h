#pragma once

#include <cstdint>
#include <string>

#include "ISemaphore.h"
#include "ISurface.h"
#include "ITexture.h"
#include "RHIFwd.h"
#include "RHITypes.h"
#include "Result.h"

namespace rhi {

/// How presented images are queued for display.
enum class PresentMode {
    Fifo,      /**< Vsync; waits for vertical blank. Always supported. */
    Mailbox,   /**< Vsync without blocking; the newest frame replaces a queued one. */
    Immediate, /**< No vsync; lowest latency, may tear. */
};

/// Describes a swapchain to create with IDevice::createSwapchain.
struct SwapchainDesc {
    SurfaceRef surface;                 /**< Surface to present to; retained. */
    Extent2D extent;                    /**< Backbuffer size, normally the window's client size. */
    Format format = Format::BGRA8Unorm; /**< Backbuffer format. */
    std::uint32_t image_count = 3;      /**< Minimum number of images; the backend may create more. */
    PresentMode present_mode = PresentMode::Fifo;
    TextureUsage usage = TextureUsage::RenderTarget; /**< How backbuffers will be used. */
    std::string label;
};

/// Parameters for ISwapchain::acquire.
struct AcquireDesc {
    /// Binary semaphore signaled when the image is ready to be rendered to; wait on it in the frame's submit.
    SemaphoreRef signal_semaphore;
    Timeout timeout = kWaitForever; /**< Maximum CPU wait for an image. */
};

/// The outcome of ISwapchain::acquire.
enum class AcquireStatus {
    Success,    /**< An image was acquired. */
    Suboptimal, /**< An image was acquired, but the swapchain should be resized soon. */
    OutOfDate,  /**< No image; call ISwapchain::resize and acquire again. */
    Timeout,    /**< No image became available in time. */
    Error,      /**< Acquisition failed (e.g. surface or device lost). */
};

/// The result of ISwapchain::acquire.
struct AcquireResult {
    AcquireStatus status = AcquireStatus::Error; /**< Outcome. */
    std::uint32_t image_index = 0;               /**< Index to pass to PresentDesc::image_index. */
    TextureRef texture; /**< The backbuffer, owned by the swapchain; valid until the next resize(). */
};

/**
 * The chain of backbuffer images presented to a surface.
 *
 * Per frame: acquire() an image, render into its texture (barrier it to ResourceState::RenderTarget, then to
 * ResourceState::Present), submit, then IQueue::present the image.
 */
class ISwapchain {
public:
    virtual ~ISwapchain() = default;

    /**
     * Acquires the next backbuffer to render into.
     *
     * @return The image and status; check AcquireResult::status before using it.
     */
    virtual AcquireResult acquire(AcquireDesc desc) = 0;

    /**
     * Recreates the backbuffers at a new size, e.g. after the window was resized.
     *
     * Backbuffer textures returned before must no longer be in use by the GPU.
     */
    virtual Result<void> resize(Extent2D extent) = 0;
};

} // namespace rhi
