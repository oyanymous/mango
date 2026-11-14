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

enum class PresentMode {
    Fifo,      // vsync; always supported
    Mailbox,   // vsync without blocking; newest frame wins
    Immediate, // no vsync; may tear
};

struct SwapchainDesc {
    SurfaceRef surface;
    Extent2D extent;
    Format format = Format::BGRA8Unorm;
    std::uint32_t image_count = 3; // minimum requested; the backend may create more
    PresentMode present_mode = PresentMode::Fifo;
    TextureUsage usage = TextureUsage::RenderTarget;
    std::string label;
};

struct AcquireDesc {
    SemaphoreRef signal_semaphore; // binary; signaled when the image is ready to be rendered to
    Timeout timeout = kWaitForever;
};

enum class AcquireStatus {
    Success,
    Suboptimal, // usable, but the swapchain should be resized
    OutOfDate,  // not usable; call resize() and acquire again
    Timeout,
    Error,
};

struct AcquireResult {
    AcquireStatus status = AcquireStatus::Error;
    std::uint32_t image_index = 0;
    TextureRef texture; // the backbuffer, owned by the swapchain; valid until the next resize()
};

class ISwapchain {
public:
    virtual ~ISwapchain() = default;

    virtual AcquireResult acquire(AcquireDesc desc) = 0;
    // Recreates the backbuffers. Previously returned backbuffer textures must no longer be in use.
    virtual Result<void> resize(Extent2D extent) = 0;
};

} // namespace rhi
