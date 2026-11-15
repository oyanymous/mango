#pragma once

#import <QuartzCore/CAMetalLayer.h>

#include <memory>
#include <vector>

#include "ISurface.h"
#include "ISwapchain.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalTexture;

class MetalSurface final : public ISurface {
public:
    static Result<SurfaceRef> create(SurfaceDesc desc);

    MetalSurface(SurfaceDesc desc, CAMetalLayer* layer);

    SurfaceDesc getDesc() const override;

    CAMetalLayer* layer() const { return m_layer; }
private:
    SurfaceDesc m_desc;
    CAMetalLayer* m_layer = nil;
};

class MetalSwapchain final : public ISwapchain {
public:
    static Result<SwapchainRef> create(std::shared_ptr<MetalDeviceContext> context, SwapchainDesc desc);

    MetalSwapchain(std::shared_ptr<MetalDeviceContext> context, SwapchainDesc desc, CAMetalLayer* layer);

    AcquireResult acquire(AcquireDesc desc) override;
    Result<void> resize(Extent2D extent) override;

    id<CAMetalDrawable> takeDrawable(std::uint32_t image_index);
private:
    void createBackbuffers();

    SwapchainDesc m_desc;
    CAMetalLayer* m_layer = nil;
    std::vector<std::shared_ptr<MetalTexture>> m_backbuffers;
    std::vector<id<CAMetalDrawable>> m_drawables;
    std::uint32_t m_next_image = 0;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
