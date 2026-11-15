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
    Fifo,
    Mailbox,
    Immediate,
};

struct SwapchainDesc {
    SurfaceRef surface;
    Extent2D extent;
    Format format = Format::BGRA8Unorm;
    std::uint32_t image_count = 3;
    PresentMode present_mode = PresentMode::Fifo;
    TextureUsage usage = TextureUsage::RenderTarget;
    std::string label;
};

struct AcquireDesc {
    SemaphoreRef signal_semaphore;
    Timeout timeout = kWaitForever;
};

enum class AcquireStatus {
    Success,
    Suboptimal,
    OutOfDate,
    Timeout,
    Error,
};

struct AcquireResult {
    AcquireStatus status = AcquireStatus::Error;
    std::uint32_t image_index = 0;
    TextureRef texture;
};

class ISwapchain {
public:
    virtual ~ISwapchain() = default;

    virtual AcquireResult acquire(AcquireDesc desc) = 0;

    virtual Result<void> resize(Extent2D extent) = 0;
};

} // namespace rhi
