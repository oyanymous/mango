#include "MetalSwapchain.h"

#include <algorithm>
#include <utility>
#include <variant>

#include "MetalSemaphore.h"
#include "MetalTexture.h"

namespace rhi::metal {

Result<SurfaceRef> MetalSurface::create(SurfaceDesc desc) {
    const auto* handle = std::get_if<MetalLayerHandle>(&desc.window);
    if (handle == nullptr || handle->layer == nullptr) {
        return Error{ErrorCode::InvalidArgument, "createSurface: the Metal backend needs a MetalLayerHandle"};
    }
    CAMetalLayer* layer = (__bridge CAMetalLayer*)handle->layer;
    if (![layer isKindOfClass:[CAMetalLayer class]]) {
        return Error{ErrorCode::InvalidArgument, "createSurface: MetalLayerHandle::layer is not a CAMetalLayer"};
    }
    return SurfaceRef(std::make_shared<MetalSurface>(std::move(desc), layer));
}

MetalSurface::MetalSurface(SurfaceDesc desc, CAMetalLayer* layer) : m_desc(std::move(desc)), m_layer(layer) {
}

SurfaceDesc MetalSurface::getDesc() const {
    return m_desc;
}

Result<SwapchainRef> MetalSwapchain::create(std::shared_ptr<MetalDeviceContext> context, SwapchainDesc desc) {
    if (!desc.surface) {
        return Error{ErrorCode::InvalidArgument, "createSwapchain: surface is null"};
    }
    if (desc.format != Format::BGRA8Unorm && desc.format != Format::BGRA8Srgb && desc.format != Format::RGBA16Float) {
        return Error{ErrorCode::Unsupported,
                     "createSwapchain: a CAMetalLayer needs BGRA8Unorm, BGRA8Srgb or RGBA16Float"};
    }
    CAMetalLayer* layer = static_cast<const MetalSurface&>(*desc.surface).layer();
    layer.device = context->device;
    layer.pixelFormat = toMTLPixelFormat(desc.format);

    layer.framebufferOnly = desc.usage == TextureUsage::RenderTarget;
    layer.maximumDrawableCount = std::clamp<std::uint32_t>(desc.image_count, 2, 3);
    layer.displaySyncEnabled = desc.present_mode != PresentMode::Immediate;
    layer.drawableSize = CGSizeMake(desc.extent.width, desc.extent.height);
    layer.name = toNSString(desc.label);
    return SwapchainRef(std::make_shared<MetalSwapchain>(std::move(context), std::move(desc), layer));
}

MetalSwapchain::MetalSwapchain(std::shared_ptr<MetalDeviceContext> context, SwapchainDesc desc, CAMetalLayer* layer)
    : m_desc(std::move(desc)), m_layer(layer), m_context(std::move(context)) {
    createBackbuffers();
}

void MetalSwapchain::createBackbuffers() {
    const auto count = static_cast<std::size_t>(m_layer.maximumDrawableCount);
    m_backbuffers.clear();
    m_drawables.assign(count, nil);
    m_next_image = 0;
    for (std::size_t i = 0; i < count; ++i) {
        m_backbuffers.push_back(
            std::make_shared<MetalTexture>(m_context,
                                           TextureDesc{
                                               .format = m_desc.format,
                                               .extent = {m_desc.extent.width, m_desc.extent.height},
                                               .usage = m_desc.usage,
                                               .label = m_desc.label,
                                           },
                                           nil));
    }
}

AcquireResult MetalSwapchain::acquire(AcquireDesc desc) {
    if (m_desc.extent.width == 0 || m_desc.extent.height == 0) {
        return {.status = AcquireStatus::OutOfDate};
    }
    const std::uint32_t index = m_next_image;
    if (m_drawables[index] != nil) {
        m_drawables[index] = nil;
    }
    id<CAMetalDrawable> drawable = [m_layer nextDrawable];
    if (drawable == nil) {
        return {.status = AcquireStatus::Timeout};
    }
    m_drawables[index] = drawable;
    m_backbuffers[index]->setHandle(drawable.texture);
    m_next_image = (index + 1) % static_cast<std::uint32_t>(m_backbuffers.size());

    if (desc.signal_semaphore) {
        static_cast<MetalSemaphore&>(*desc.signal_semaphore).signalFromHost();
    }
    return {.status = AcquireStatus::Success, .image_index = index, .texture = m_backbuffers[index]};
}

Result<void> MetalSwapchain::resize(Extent2D extent) {
    m_desc.extent = extent;
    if (extent.width != 0 && extent.height != 0) {
        m_layer.drawableSize = CGSizeMake(extent.width, extent.height);
    }

    createBackbuffers();
    return {};
}

id<CAMetalDrawable> MetalSwapchain::takeDrawable(std::uint32_t image_index) {
    if (image_index >= m_drawables.size()) {
        return nil;
    }
    id<CAMetalDrawable> drawable = m_drawables[image_index];
    m_drawables[image_index] = nil;
    return drawable;
}

} // namespace rhi::metal
