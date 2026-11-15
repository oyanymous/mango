#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

#include "Application.h"
#include "RHI.h"
#include "Window.h"
#include "triangle.frag.h"
#include "triangle.vert.h"

namespace {

constexpr std::size_t kFramesInFlight = 2;
constexpr rhi::Format kBackbufferFormat = rhi::Format::BGRA8Unorm;

struct Vertex {
    float x, y;
    float u, v;
};

constexpr std::array<Vertex, 4> kVertices = {{
    {-0.6f, -0.6f, 0.0f, 0.0f},
    {0.6f, -0.6f, 1.0f, 0.0f},
    {0.6f, 0.6f, 1.0f, 1.0f},
    {-0.6f, 0.6f, 0.0f, 1.0f},
}};

constexpr std::array<std::uint16_t, 6> kIndices = {0, 1, 2, 2, 3, 0};

struct Transform {
    float cos_angle, sin_angle;
    float x_scale;
    float padding;
};

constexpr Transform kNoTransform = {1.0f, 0.0f, 1.0f, 0.0f};

struct Frame {
    rhi::CommandBufferRef command_buffer;
    rhi::FenceRef fence;
    rhi::SemaphoreRef image_ready;
    rhi::BufferRef uniforms;
    rhi::BindGroupRef bind_group;
};

struct Renderer {
    rhi::DeviceRef device;
    rhi::QueueRef queue;
    rhi::SwapchainRef swapchain;
    rhi::RenderPassRef render_pass;
    rhi::BindGroupLayoutRef bind_group_layout;
    rhi::PipelineRef pipeline;
    rhi::TextureViewRef texture_view;
    rhi::SamplerRef sampler;
    rhi::BufferRef vertices;
    rhi::BufferRef indices;
    std::array<Frame, kFramesInFlight> frames;
    std::size_t frame_index = 0;

    std::vector<rhi::SemaphoreRef> render_finished;
    std::vector<rhi::RenderTargetRef> targets;
    bool needs_resize = false;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
};

template <typename T>
T orExit(rhi::Result<T> result, const char* what) {
    if (!result) {
        std::fprintf(stderr, "%s failed: %s\n", what, result.error().message.c_str());
        std::exit(EXIT_FAILURE);
    }
    if constexpr (!std::is_void_v<T>) {
        return std::move(result).value();
    }
}

rhi::BufferRef createFilledBuffer(rhi::IDevice& device, std::span<const std::byte> bytes, rhi::BufferUsage usage) {
    rhi::BufferRef buffer =
        orExit(device.createBuffer({.size = bytes.size(), .usage = usage, .memory = rhi::MemoryLocation::CpuToGpu}),
               "createBuffer");
    std::span<std::byte> mapped = orExit(buffer->map({}), "map");
    std::memcpy(mapped.data(), bytes.data(), bytes.size());
    buffer->unmap();
    return buffer;
}

rhi::PipelineRef createPipeline(rhi::IDevice& device, const rhi::RenderPassRef& render_pass,
                                const rhi::BindGroupLayoutRef& bind_group_layout) {
    auto vertex_shader =
        orExit(device.createShader({.stage = rhi::ShaderStage::Vertex,
                                    .spirv = {std::begin(triangle_vert_spv), std::end(triangle_vert_spv)}}),
               "createShader(vertex)");
    auto fragment_shader =
        orExit(device.createShader({.stage = rhi::ShaderStage::Fragment,
                                    .spirv = {std::begin(triangle_frag_spv), std::end(triangle_frag_spv)}}),
               "createShader(fragment)");
    auto layout =
        orExit(device.createPipelineLayout({.bind_group_layouts = {bind_group_layout}}), "createPipelineLayout");
    return orExit(device.createGraphicsPipeline({
                      .layout = layout,
                      .vertex_shader = vertex_shader,
                      .fragment_shader = fragment_shader,
                      .vertex_buffers = {{.stride = sizeof(Vertex),
                                          .attributes = {{0, rhi::Format::RG32Float, offsetof(Vertex, x)},
                                                         {1, rhi::Format::RG32Float, offsetof(Vertex, u)}}}},
                      .render_pass = render_pass,
                  }),
                  "createGraphicsPipeline");
}

rhi::TextureRef createCheckerTexture(rhi::IDevice& device, rhi::IQueue& queue) {
    constexpr std::uint32_t kSize = 64;
    std::vector<std::uint8_t> texels(kSize * kSize * 4);
    for (std::uint32_t y = 0; y < kSize; ++y) {
        for (std::uint32_t x = 0; x < kSize; ++x) {
            const bool light = ((x / 8) + (y / 8)) % 2 == 0;
            std::uint8_t* texel = &texels[(y * kSize + x) * 4];
            texel[0] = static_cast<std::uint8_t>(x * 4);
            texel[1] = static_cast<std::uint8_t>(y * 4);
            texel[2] = light ? 255 : 40;
            texel[3] = 255;
        }
    }
    auto staging = createFilledBuffer(device, std::as_bytes(std::span(texels)), rhi::BufferUsage::CopySrc);
    auto texture = orExit(device.createTexture({.format = rhi::Format::RGBA8Unorm,
                                                .extent = {kSize, kSize},
                                                .usage = rhi::TextureUsage::Sampled | rhi::TextureUsage::CopyDst}),
                          "createTexture");

    auto cmd = orExit(device.createCommandBuffer({}), "createCommandBuffer");
    orExit(cmd->begin(), "upload begin");
    cmd->barrier(
        {.textures = {
             {.texture = texture, .before = rhi::ResourceState::Undefined, .after = rhi::ResourceState::CopyDst}}});
    {
        rhi::CopyEncoderRef copy = cmd->beginCopy();
        copy->copyBufferTexture(
            {.buffer = {.buffer = staging}, .texture = {.texture = texture}, .extent = {kSize, kSize}});
        copy->end();
    }
    cmd->barrier(
        {.textures = {
             {.texture = texture, .before = rhi::ResourceState::CopyDst, .after = rhi::ResourceState::ShaderRead}}});
    orExit(cmd->end(), "upload end");
    auto fence = orExit(device.createFence({}), "createFence");
    orExit(queue.submit({.command_buffers = {cmd}, .fence = fence}), "upload submit");
    orExit(fence->wait(rhi::kWaitForever), "upload wait");
    return texture;
}

void resize(Renderer& r, const Window& window) {
    const WindowSize size = window.clientSize();
    r.targets.clear();
    if (rhi::Result<void> result =
            r.swapchain->resize({static_cast<std::uint32_t>(size.width), static_cast<std::uint32_t>(size.height)});
        !result) {
        std::fprintf(stderr, "swapchain resize failed: %s\n", result.error().message.c_str());
        std::exit(EXIT_FAILURE);
    }
    r.needs_resize = false;
}

rhi::RenderTargetRef targetFor(Renderer& r, std::uint32_t image_index, const rhi::TextureRef& backbuffer) {
    if (r.targets.size() <= image_index) {
        r.targets.resize(image_index + 1);
    }
    if (!r.targets[image_index]) {
        const rhi::TextureDesc desc = backbuffer->getDesc();
        auto view = orExit(r.device->createTextureView({.texture = backbuffer}), "createTextureView");
        r.targets[image_index] =
            orExit(r.device->createRenderTarget({.render_pass = r.render_pass,
                                                 .color_attachments = {view},
                                                 .extent = {desc.extent.width, desc.extent.height}}),
                   "createRenderTarget");
    }
    return r.targets[image_index];
}

rhi::SemaphoreRef renderFinishedFor(Renderer& r, std::uint32_t image_index) {
    if (r.render_finished.size() <= image_index) {
        r.render_finished.resize(image_index + 1);
    }
    if (!r.render_finished[image_index]) {
        r.render_finished[image_index] = orExit(r.device->createSemaphore({}), "createSemaphore");
    }
    return r.render_finished[image_index];
}

void renderFrame(Renderer& r, const Window& window) {
    const WindowSize size = window.clientSize();
    if (size.width == 0 || size.height == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        r.needs_resize = true;
        return;
    }
    if (r.needs_resize) {
        resize(r, window);
    }

    Frame& frame = r.frames[r.frame_index];
    orExit(frame.fence->wait(rhi::kWaitForever), "fence wait");

    rhi::AcquireResult acquired = r.swapchain->acquire({.signal_semaphore = frame.image_ready});
    if (acquired.status == rhi::AcquireStatus::OutOfDate) {
        resize(r, window);
        return;
    }
    if (acquired.status != rhi::AcquireStatus::Success && acquired.status != rhi::AcquireStatus::Suboptimal) {
        std::fprintf(stderr, "swapchain acquire failed\n");
        std::exit(EXIT_FAILURE);
    }
    if (acquired.status == rhi::AcquireStatus::Suboptimal) {
        r.needs_resize = true;
    }

    const float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - r.start).count();
    const std::array<float, 4> color = {0.2f + 0.2f * std::sin(t), 0.2f + 0.2f * std::sin(t + 2.094f),
                                        0.2f + 0.2f * std::sin(t + 4.189f), 1.0f};

    const Transform transform{std::cos(t * 0.8f), std::sin(t * 0.8f),
                              static_cast<float>(size.height) / static_cast<float>(size.width), 0.0f};
    std::span<std::byte> mapped = orExit(frame.uniforms->map({}), "map uniforms");
    std::memcpy(mapped.data(), &transform, sizeof(transform));
    frame.uniforms->unmap();

    orExit(frame.fence->reset(), "fence reset");
    rhi::ICommandBuffer& cmd = *frame.command_buffer;
    orExit(cmd.begin(), "command buffer begin");
    cmd.barrier({.textures = {{.texture = acquired.texture,
                               .before = rhi::ResourceState::Undefined,
                               .after = rhi::ResourceState::RenderTarget}}});
    {
        rhi::RenderEncoderRef pass = cmd.beginRender({.render_pass = r.render_pass,
                                                      .target = targetFor(r, acquired.image_index, acquired.texture),
                                                      .clear_colors = {color}});
        pass->setPipeline(r.pipeline);
        pass->setBindGroup({.index = 0, .bind_group = frame.bind_group});
        pass->setVertexBuffers({.buffers = {{r.vertices}}});
        pass->setIndexBuffer({.buffer = r.indices, .format = rhi::IndexFormat::Uint16});
        pass->drawIndexed({.index_count = static_cast<std::uint32_t>(kIndices.size())});
        pass->end();
    }
    cmd.barrier({.textures = {{.texture = acquired.texture,
                               .before = rhi::ResourceState::RenderTarget,
                               .after = rhi::ResourceState::Present}}});
    orExit(cmd.end(), "command buffer end");

    rhi::SemaphoreRef render_finished = renderFinishedFor(r, acquired.image_index);
    orExit(r.queue->submit({
               .command_buffers = {frame.command_buffer},
               .waits = {{frame.image_ready, 0, rhi::PipelineStage::RenderTargetOutput}},
               .signals = {{render_finished, 0}},
               .fence = frame.fence,
           }),
           "submit");

    rhi::Result<void> presented = r.queue->present(
        {.swapchain = r.swapchain, .image_index = acquired.image_index, .wait_semaphores = {render_finished}});
    if (!presented) {
        if (presented.error().code != rhi::ErrorCode::OutOfDate) {
            std::fprintf(stderr, "present failed: %s\n", presented.error().message.c_str());
            std::exit(EXIT_FAILURE);
        }
        r.needs_resize = true;
    }
    r.frame_index = (r.frame_index + 1) % kFramesInFlight;
}

rhi::Backend chooseBackend(int argc, char** argv, const rhi::RHIFactory& factory) {
    const rhi::BackendList available = factory.availableBackends();
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string_view(argv[i]) != "--backend") {
            continue;
        }
        const std::string_view name = argv[i + 1];
        for (rhi::Backend backend : available) {
            std::string lower(rhi::toString(backend));
            for (char& c : lower) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (name == lower) {
                return backend;
            }
        }
        std::fprintf(stderr, "backend '%.*s' is not available\n", static_cast<int>(name.size()), name.data());
        std::exit(EXIT_FAILURE);
    }
    if (available.empty()) {
        std::fprintf(stderr, "no RHI backend is available\n");
        std::exit(EXIT_FAILURE);
    }
    return available[0];
}

std::uint64_t frameLimit(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string_view(argv[i]) == "--frames") {
            return std::strtoull(argv[i + 1], nullptr, 10);
        }
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    Application app;
    rhi::RHIFactory factory;
    const rhi::Backend backend = chooseBackend(argc, argv, factory);
    std::printf("Backend: %s\n", std::string(rhi::toString(backend)).c_str());

    int problems = 0;
    auto instance = orExit(factory.createInstance(backend,
                                                  {
                                                      .application_name = "rhi_triangle",
                                                      .enable_validation = true,
                                                      .debug_callback =
                                                          [&problems](const rhi::DebugMessage& message) {
                                                              if (message.severity == rhi::DebugSeverity::Warning ||
                                                                  message.severity == rhi::DebugSeverity::Error) {
                                                                  ++problems;
                                                              }
                                                              std::printf("[debug] %.*s\n",
                                                                          static_cast<int>(message.message.size()),
                                                                          message.message.data());
                                                          },
                                                  }),
                           "createInstance");

    Window window("rhi_triangle");
    window.display();
    const NativeWindow native = window.nativeWindow();
#if defined(__APPLE__)
    const rhi::NativeWindowHandle handle = rhi::MetalLayerHandle{.layer = native.metal_layer};
#else
    const rhi::NativeWindowHandle handle = rhi::Win32WindowHandle{.hinstance = native.hinstance, .hwnd = native.hwnd};
#endif
    auto surface = orExit(instance->createSurface({.window = handle}), "createSurface");

    rhi::AdapterList adapters = instance->enumerateAdapters();
    if (adapters.empty()) {
        std::fprintf(stderr, "no adapter\n");
        return EXIT_FAILURE;
    }
    std::printf("Adapter: %s\n", adapters[0]->getCapabilities().name.c_str());

    {
        Renderer r;
        r.device = orExit(adapters[0]->createDevice({}), "createDevice");
        r.queue = r.device->getQueue(rhi::QueueType::Graphics);
        const WindowSize size = window.clientSize();
        r.swapchain = orExit(r.device->createSwapchain({.surface = surface,
                                                        .extent = {static_cast<std::uint32_t>(size.width),
                                                                   static_cast<std::uint32_t>(size.height)},
                                                        .format = kBackbufferFormat}),
                             "createSwapchain");
        r.render_pass = orExit(r.device->createRenderPass({.color_attachments = {{.format = kBackbufferFormat}}}),
                               "createRenderPass");
        r.bind_group_layout = orExit(
            r.device->createBindGroupLayout({.entries =
                                                 {
                                                     {0, rhi::BindingType::UniformBuffer, rhi::ShaderStage::Vertex},
                                                     {1, rhi::BindingType::SampledTexture, rhi::ShaderStage::Fragment},
                                                     {2, rhi::BindingType::Sampler, rhi::ShaderStage::Fragment},
                                                 }}),
            "createBindGroupLayout");
        r.pipeline = createPipeline(*r.device, r.render_pass, r.bind_group_layout);
        r.vertices = createFilledBuffer(*r.device, std::as_bytes(std::span(kVertices)), rhi::BufferUsage::Vertex);
        r.indices = createFilledBuffer(*r.device, std::as_bytes(std::span(kIndices)), rhi::BufferUsage::Index);
        r.texture_view = orExit(r.device->createTextureView({.texture = createCheckerTexture(*r.device, *r.queue)}),
                                "createTextureView");
        r.sampler = orExit(r.device->createSampler({}), "createSampler");
        for (Frame& frame : r.frames) {
            frame.command_buffer = orExit(r.device->createCommandBuffer({}), "createCommandBuffer");
            frame.fence = orExit(r.device->createFence({.signaled = true}), "createFence");
            frame.image_ready = orExit(r.device->createSemaphore({}), "createSemaphore");
            frame.uniforms =
                createFilledBuffer(*r.device, std::as_bytes(std::span(&kNoTransform, 1)), rhi::BufferUsage::Uniform);
            frame.bind_group = orExit(r.device->createBindGroup({.layout = r.bind_group_layout,
                                                                 .entries =
                                                                     {
                                                                         {.binding = 0, .buffer = frame.uniforms},
                                                                         {.binding = 1, .texture_view = r.texture_view},
                                                                         {.binding = 2, .sampler = r.sampler},
                                                                     }}),
                                      "createBindGroup");
        }

        window.setResizeCallback([&r](WindowSize) { r.needs_resize = true; });
        std::uint64_t frames = 0;
        const std::uint64_t frame_limit = frameLimit(argc, argv);
        Application::setFrameCallback([&] {
            renderFrame(r, window);
            if (++frames == frame_limit) {
                Application::quit();
            }
        });
        app.run();

        Application::setFrameCallback({});
        window.setResizeCallback({});

        std::printf("Rendered %llu frames\n", static_cast<unsigned long long>(frames));
    }

    std::printf("%d validation warning(s)/error(s)\n", problems);
    return problems == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
