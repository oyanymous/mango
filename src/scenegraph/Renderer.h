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

struct RendererDesc {
    rhi::DeviceRef device;
    rhi::SurfaceRef surface;
    rhi::Extent2D size;
    float device_pixel_ratio = 1.0f;
    ColorF clear_color = ColorF::white();
    rhi::PresentMode present_mode = rhi::PresentMode::Fifo;

    std::function<void()> request_frame;
    std::string label;
};

struct FrameStats {
    std::uint64_t frame = 0;
    bool skipped = false;
    bool full_redraw = false;
    std::uint32_t nodes_visited = 0;
    std::uint32_t batches = 0;
    bool batches_rebuilt = false;
    std::uint32_t batch_draws = 0;
    std::uint32_t draw_calls = 0;
    std::uint32_t layers_rendered = 0;
    std::uint64_t bytes_uploaded = 0;
    std::uint32_t damage_rects = 0;
    std::int64_t damage_area = 0;
};

class Renderer {
public:
    static rhi::Result<std::unique_ptr<Renderer>> create(RendererDesc desc);

    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void setRootNode(RootNode* root);

    [[nodiscard]] RootNode* rootNode() const;

    void resize(rhi::Extent2D size, float device_pixel_ratio);

    [[nodiscard]] rhi::Extent2D size() const;

    void setClearColor(ColorF color);

    [[nodiscard]] bool needsRender() const;

    rhi::Result<FrameStats> render();

    [[nodiscard]] const FrameStats& lastFrameStats() const;

    rhi::Result<std::vector<std::uint8_t>> readPixels();
private:
    struct Impl;
    explicit Renderer(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;
};

} // namespace sg
