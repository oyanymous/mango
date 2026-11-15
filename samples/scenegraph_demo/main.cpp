/**
 * @file
 * @brief scenegraph_demo: a window drawn by sg::Renderer, with a few hundred nodes, some of them animated.
 *
 * The scene has a static grid of rectangles, a spinning rotated clip with content, a layer of 400 rectangles that
 * drifts across the window (moved as one cached texture), a textured image and a polyline. Only the animated parts are
 * redrawn each frame; the frame callback runs on demand, so with `--static` (no animation) the application sleeps
 * between events.
 *
 * Validation is on; the program exits non-zero if validation reported anything.
 *
 * Usage: `scenegraph_demo [--backend vulkan|metal] [--frames N] [--static] [--screenshot FILE.ppm]`. `--frames N`
 * quits after N frames; `--screenshot` then saves the last frame's canvas as a binary PPM image.
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "Application.h"
#include "ClipNode.h"
#include "GeometryNode.h"
#include "LayerNode.h"
#include "OpacityNode.h"
#include "RHI.h"
#include "Renderer.h"
#include "TransformNode.h"
#include "Window.h"

namespace {

/// Whether @p flag appears among the arguments.
bool hasFlag(int argc, char** argv, std::string_view flag) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == flag) {
            return true;
        }
    }
    return false;
}

/// The value after @p flag, or null.
const char* flagValue(int argc, char** argv, std::string_view flag) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == flag) {
            return argv[i + 1];
        }
    }
    return nullptr;
}

/// The backend named by `--backend`, or the first available one. Exits on an unknown or unavailable name.
rhi::Backend chooseBackend(int argc, char** argv, const rhi::RHIFactory& factory) {
    const rhi::BackendList available = factory.availableBackends();
    if (const char* name = flagValue(argc, argv, "--backend")) {
        for (rhi::Backend backend : available) {
            std::string lower(rhi::toString(backend));
            for (char& c : lower) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (lower == name) {
                return backend;
            }
        }
        std::fprintf(stderr, "backend '%s' is not available\n", name);
        std::exit(EXIT_FAILURE);
    }
    if (available.empty()) {
        std::fprintf(stderr, "no RHI backend is available\n");
        std::exit(EXIT_FAILURE);
    }
    return available[0];
}

/// Writes premultiplied RGBA8 pixels (opaque here, so premultiplication does not matter) as a binary PPM.
bool writePpm(const char* path, const std::vector<std::uint8_t>& rgba, std::uint32_t width, std::uint32_t height) {
    std::FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }
    std::fprintf(file, "P6\n%u %u\n255\n", width, height);
    for (std::size_t i = 0; i + 3 < rgba.size(); i += 4) {
        std::fwrite(&rgba[i], 1, 3, file);
    }
    return std::fclose(file) == 0;
}

/// A 64x64 checkerboard texture.
std::shared_ptr<sg::Texture> checkerboard() {
    constexpr std::uint32_t kSize = 64;
    std::vector<std::uint8_t> pixels(kSize * kSize * 4);
    for (std::uint32_t y = 0; y < kSize; ++y) {
        for (std::uint32_t x = 0; x < kSize; ++x) {
            const bool light = ((x / 8) + (y / 8)) % 2 == 0;
            std::uint8_t* p = &pixels[(y * kSize + x) * 4];
            p[0] = static_cast<std::uint8_t>(x * 4);
            p[1] = static_cast<std::uint8_t>(y * 4);
            p[2] = light ? 255 : 60;
            p[3] = 255;
        }
    }
    return sg::Texture::fromRgba8(kSize, kSize, std::move(pixels));
}

/// The animated parts of the scene.
struct Scene {
    sg::RootNode root;
    sg::TransformNode* fit = nullptr;     // scales the 900 x 600 design to the window
    sg::TransformNode* spinner = nullptr; // rotates a clip
    sg::TransformNode* drifter = nullptr; // moves the layer
    sg::OpacityNode* pulse = nullptr;     // fades a rectangle
};

/// Builds the demo scene, in logical units (the window starts at 900 x 600).
void buildScene(Scene& scene) {
    scene.fit = scene.root.emplaceChild<sg::TransformNode>();
    sg::Node& top = *scene.fit;
    // Static background grid: 300 rectangles in one batch.
    for (int i = 0; i < 300; ++i) {
        const float x = 20.0f + (i % 20) * 22.0f;
        const float y = 20.0f + (i / 20) * 22.0f;
        top.emplaceChild<sg::RectangleNode>(
            sg::RectF{x, y, 18, 18}, sg::ColorF{0.85f - (i % 20) * 0.02f, 0.88f, 0.95f - (i / 20) * 0.03f, 1.0f});
    }

    // A rotating clip (stencil) around a striped square.
    scene.spinner = top.emplaceChild<sg::TransformNode>(sg::Transform2D::translation(640, 160));
    auto* clip = scene.spinner->emplaceChild<sg::ClipNode>(sg::RectF{-80, -80, 160, 160});
    auto* counter_rotation = clip->emplaceChild<sg::TransformNode>();
    for (int i = 0; i < 12; ++i) {
        counter_rotation->emplaceChild<sg::RectangleNode>(sg::RectF{-120.0f + i * 20.0f, -120, 10, 240},
                                                          sg::ColorF{0.9f, 0.4f + i * 0.04f, 0.1f, 1.0f});
    }

    // A layer of 400 small rectangles that drifts as one texture.
    scene.drifter = top.emplaceChild<sg::TransformNode>(sg::Transform2D::translation(40, 380));
    auto* layer = scene.drifter->emplaceChild<sg::LayerNode>();
    for (int i = 0; i < 400; ++i) {
        layer->emplaceChild<sg::RectangleNode>(sg::RectF{(i % 40) * 6.0f, (i / 40) * 6.0f, 5, 5},
                                               sg::ColorF{(i % 40) / 40.0f, 0.3f, 1.0f - (i / 40) / 10.0f, 1.0f});
    }

    // An image, a fading rectangle on top of it, and a polyline.
    top.emplaceChild<sg::ImageNode>(sg::RectF{560, 360, 128, 128}, checkerboard());
    scene.pulse = top.emplaceChild<sg::OpacityNode>(1.0f);
    scene.pulse->emplaceChild<sg::RectangleNode>(sg::RectF{620, 420, 120, 120}, sg::ColorF{0.1f, 0.6f, 0.3f, 1.0f});
    std::vector<sg::PointF> wave;
    for (int i = 0; i <= 60; ++i) {
        wave.push_back({500.0f + i * 6.0f, 560.0f + 20.0f * std::sin(i * 0.3f)});
    }
    top.emplaceChild<sg::GeometryNode>(sg::Geometry::polyline(wave, 3.0f),
                                       sg::Material::flatColor({0.2f, 0.2f, 0.8f, 1.0f}));
}

/// Scales the 900 x 600 design to fit a window of @p size pixels at @p dpr pixels per unit.
void fitToWindow(Scene& scene, WindowSize size, float dpr) {
    const float scale = std::min(size.width / dpr / 900.0f, size.height / dpr / 600.0f);
    if (scale > 0.0f) {
        scene.fit->setMatrix(sg::Transform2D::scaling(scale, scale));
    }
}

/// Moves the animated nodes to their state at frame @p frame.
void animate(Scene& scene, std::uint64_t frame) {
    const float t = static_cast<float>(frame) / 60.0f;
    scene.spinner->setMatrix(sg::Transform2D::translation(640, 160) * sg::Transform2D::rotation(t * 0.8f));
    static_cast<sg::TransformNode*>(scene.spinner->children()[0]->children()[0].get())
        ->setMatrix(sg::Transform2D::rotation(-t * 1.6f));
    scene.drifter->setMatrix(sg::Transform2D::translation(40.0f + 120.0f * (0.5f + 0.5f * std::sin(t * 0.7f)), 380));
    scene.pulse->setOpacity(0.5f + 0.5f * std::sin(t * 2.0f));
}

} // namespace

int main(int argc, char** argv) {
    Application app;
    rhi::RHIFactory factory;
    const rhi::Backend backend = chooseBackend(argc, argv, factory);
    std::printf("Backend: %s\n", std::string(rhi::toString(backend)).c_str());

    // Before the window: on macOS creating a window initializes Metal, after which validation cannot be turned on.
    int problems = 0;
    auto instance = factory.createInstance(
        backend,
        {.application_name = "scenegraph_demo",
         .enable_validation = true,
         .debug_callback = [&problems](const rhi::DebugMessage& message) {
             if (message.severity == rhi::DebugSeverity::Warning || message.severity == rhi::DebugSeverity::Error) {
                 ++problems;
             }
             std::printf("[debug] %.*s\n", static_cast<int>(message.message.size()), message.message.data());
         }});
    if (!instance) {
        std::fprintf(stderr, "createInstance failed: %s\n", instance.error().message.c_str());
        return EXIT_FAILURE;
    }

    Window window("scenegraph_demo");
    window.display();
    const NativeWindow native = window.nativeWindow();
#if defined(__APPLE__)
    const rhi::NativeWindowHandle handle = rhi::MetalLayerHandle{.layer = native.metal_layer};
#else
    const rhi::NativeWindowHandle handle = rhi::Win32WindowHandle{.hinstance = native.hinstance, .hwnd = native.hwnd};
#endif
    auto surface = (*instance)->createSurface({.window = handle});
    rhi::AdapterList adapters = (*instance)->enumerateAdapters();
    if (!surface || adapters.empty()) {
        std::fprintf(stderr, "no surface or adapter\n");
        return EXIT_FAILURE;
    }
    auto device = adapters[0]->createDevice({});
    if (!device) {
        std::fprintf(stderr, "createDevice failed: %s\n", device.error().message.c_str());
        return EXIT_FAILURE;
    }

    const bool animated = !hasFlag(argc, argv, "--static");
    const char* frames_value = flagValue(argc, argv, "--frames");
    const std::uint64_t frame_limit = frames_value != nullptr ? std::strtoull(frames_value, nullptr, 10) : 0;
    {
        const WindowSize size = window.clientSize();
        auto renderer = sg::Renderer::create({
            .device = *device,
            .surface = *surface,
            .size = {static_cast<std::uint32_t>(size.width), static_cast<std::uint32_t>(size.height)},
            .device_pixel_ratio = window.devicePixelRatio(),
            .clear_color = {0.97f, 0.97f, 0.98f, 1.0f},
            .request_frame = [] { Application::requestFrame(); },
            .label = "demo",
        });
        if (!renderer) {
            std::fprintf(stderr, "renderer creation failed: %s\n", renderer.error().message.c_str());
            return EXIT_FAILURE;
        }
        Scene scene;
        buildScene(scene);
        fitToWindow(scene, size, window.devicePixelRatio());
        (*renderer)->setRootNode(&scene.root);

        window.setResizeCallback([&](WindowSize size) {
            (*renderer)->resize({static_cast<std::uint32_t>(size.width), static_cast<std::uint32_t>(size.height)},
                                window.devicePixelRatio());
            fitToWindow(scene, size, window.devicePixelRatio());
        });
        std::uint64_t frames = 0;
        std::uint64_t skipped = 0;
        std::uint64_t uploaded = 0;
        int exit_code = EXIT_SUCCESS;
        Application::setFrameCallback(
            [&] {
                if (animated) {
                    animate(scene, frames);
                    Application::requestFrame(); // keep animating
                }
                rhi::Result<sg::FrameStats> stats = (*renderer)->render();
                if (!stats) {
                    std::fprintf(stderr, "render failed: %s\n", stats.error().message.c_str());
                    exit_code = EXIT_FAILURE;
                    Application::quit();
                    return;
                }
                skipped += stats->skipped ? 1 : 0;
                uploaded += stats->bytes_uploaded;
                if (frames % 120 == 0) {
                    std::printf("frame %llu: %u batches, %u draws, %u layers re-rendered, %llu bytes uploaded, "
                                "damage %lld px in %u rect(s)%s\n",
                                static_cast<unsigned long long>(stats->frame), stats->batches, stats->draw_calls,
                                stats->layers_rendered, static_cast<unsigned long long>(stats->bytes_uploaded),
                                static_cast<long long>(stats->damage_area), stats->damage_rects,
                                stats->skipped ? " (skipped)" : "");
                }
                if (++frames == frame_limit) {
                    if (const char* path = flagValue(argc, argv, "--screenshot")) {
                        auto pixels = (*renderer)->readPixels();
                        const rhi::Extent2D extent = (*renderer)->size();
                        if (!pixels || !writePpm(path, *pixels, extent.width, extent.height)) {
                            std::fprintf(stderr, "could not save the screenshot\n");
                            exit_code = EXIT_FAILURE;
                        }
                    }
                    Application::quit();
                }
            },
            FrameMode::OnDemand);
        Application::requestFrame();
        app.run();

        Application::setFrameCallback({});
        window.setResizeCallback({});
        std::printf("Rendered %llu frames (%llu skipped), %llu bytes uploaded in total\n",
                    static_cast<unsigned long long>(frames), static_cast<unsigned long long>(skipped),
                    static_cast<unsigned long long>(uploaded));
        if (exit_code != EXIT_SUCCESS) {
            return exit_code;
        }
    }
    std::printf("%d validation warning(s)/error(s)\n", problems);
    return problems == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
