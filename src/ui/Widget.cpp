/**
 * @file
 * @brief Widget implementation, and the GPU device shared by every top-level widget's renderer.
 */

#include "Widget.h"

#include <algorithm>
#include <cstdio>

#include "Application.h"
#include "ClipNode.h"
#include "GeometryNode.h"
#include "RHI.h"
#include "Renderer.h"
#include "TransformNode.h"
#include "Window.h"

namespace {

/**
 * @brief The RHI instance and device every widget renderer uses, and the list of renderers to drive.
 *
 * @details
 * Created on the first top-level display() with the first available backend. The application's frame callback
 * (on demand) renders every registered renderer that has something to draw.
 */
struct RenderContext {
    rhi::InstanceRef instance;
    rhi::DeviceRef device;
    std::vector<sg::Renderer*> renderers;
    bool callback_installed = false;

    static RenderContext& get() {
        static RenderContext context;
        return context;
    }

    /** @brief Creates the instance and device on first use; false (with a message) if no GPU is usable. */
    bool ensureDevice() {
        if (device) {
            return true;
        }
        rhi::RHIFactory factory;
        const rhi::BackendList backends = factory.availableBackends();
        if (backends.empty()) {
            std::fprintf(stderr, "Widget: no GPU backend is available; widgets are not drawn\n");
            return false;
        }
        auto created = factory.createInstance(backends[0], {.application_name = "Mango"});
        if (!created) {
            std::fprintf(stderr, "Widget: %s\n", created.error().message.c_str());
            return false;
        }
        rhi::AdapterList adapters = (*created)->enumerateAdapters();
        if (adapters.empty()) {
            std::fprintf(stderr, "Widget: no GPU adapter\n");
            return false;
        }
        auto created_device = adapters[0]->createDevice({});
        if (!created_device) {
            std::fprintf(stderr, "Widget: %s\n", created_device.error().message.c_str());
            return false;
        }
        instance = *created;
        device = *created_device;
        return true;
    }

    void add(sg::Renderer* renderer) {
        renderers.push_back(renderer);
        if (!callback_installed) {
            Application::setFrameCallback([] { RenderContext::get().renderAll(); }, FrameMode::OnDemand);
            callback_installed = true;
        }
        Application::requestFrame();
    }

    void remove(sg::Renderer* renderer) { std::erase(renderers, renderer); }

    void renderAll() {
        for (sg::Renderer* renderer : renderers) {
            if (!renderer->needsRender()) {
                continue;
            }
            if (rhi::Result<sg::FrameStats> stats = renderer->render(); !stats) {
                std::fprintf(stderr, "Widget: rendering failed: %s\n", stats.error().message.c_str());
            }
        }
    }
};

/** @brief A UI colour as a scene graph colour. */
sg::ColorF toColorF(const Color& color) {
    return {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
}

} // namespace

Widget::Widget(Widget* parent) : m_parent(parent) {
    auto node = std::make_unique<sg::TransformNode>();
    m_node = node.get();
    m_background = m_node->emplaceChild<sg::RectangleNode>(sg::RectF{}, sg::ColorF{0, 0, 0, 0});
    m_clip = m_node->emplaceChild<sg::ClipNode>();
    m_content = m_clip->emplaceChild<sg::Node>();
    if (isTopLevel()) {
        m_root = std::make_unique<sg::RootNode>();
        m_root->appendChild(std::move(node));
        m_background->setColor(sg::ColorF::white());
        m_width = 400.0f;
        m_height = 300.0f;
        Application::addTopLevelWidget(this);
    } else {
        m_parent->m_children.push_back(this);
        m_parent->m_clip->appendChild(std::move(node));
    }
    updateNodes();
}

Widget::~Widget() {
    if (m_renderer) {
        RenderContext::get().remove(m_renderer.get());
        m_renderer.reset();
    }
    for (Widget* child : m_children) {
        child->m_parent = nullptr;
        child->detachFromDestroyedParent();
    }
    if (m_parent != nullptr) {
        std::erase(m_parent->m_children, this);
        if (m_node != nullptr) {
            m_parent->m_clip->removeChild(m_node); // destroys the subtree
        }
    }
    m_root.reset();
    m_window.reset();
}

void Widget::detachFromDestroyedParent() {
    m_node = nullptr;
    m_background = nullptr;
    m_clip = nullptr;
    m_content = nullptr;
    for (Widget* child : m_children) {
        child->detachFromDestroyedParent();
    }
}

void Widget::display() {
    if (!isTopLevel() || !m_root) {
        return; // child widgets are drawn as part of their window (and orphans not at all)
    }
    if (m_window == nullptr) {
        // Created on first display so a widget can be configured (e.g. titled) before its native window exists.
        createWindow();
    }
    m_window->display();
}

void Widget::createWindow() {
    // The device first: on macOS, the first window initializes Metal.
    const bool has_device = RenderContext::get().ensureDevice();
    m_window = std::make_unique<Window>();
    m_window->setWindowTitle(m_window_title);
    if (!has_device) {
        return;
    }
    RenderContext& context = RenderContext::get();
    const NativeWindow native = m_window->nativeWindow();
#if defined(__APPLE__)
    const rhi::NativeWindowHandle handle = rhi::MetalLayerHandle{.layer = native.metal_layer};
#else
    const rhi::NativeWindowHandle handle = rhi::Win32WindowHandle{.hinstance = native.hinstance, .hwnd = native.hwnd};
#endif
    auto surface = context.instance->createSurface({.window = handle});
    if (!surface) {
        std::fprintf(stderr, "Widget: %s\n", surface.error().message.c_str());
        return;
    }
    const WindowSize size = m_window->clientSize();
    const float dpr = m_window->devicePixelRatio();
    auto renderer = sg::Renderer::create({
        .device = context.device,
        .surface = *surface,
        .size = {static_cast<std::uint32_t>(size.width), static_cast<std::uint32_t>(size.height)},
        .device_pixel_ratio = dpr,
        .request_frame = [] { Application::requestFrame(); },
        .label = "widget",
    });
    if (!renderer) {
        std::fprintf(stderr, "Widget: %s\n", renderer.error().message.c_str());
        return;
    }
    m_renderer = std::move(renderer).value();
    m_renderer->setRootNode(m_root.get());
    m_width = static_cast<float>(size.width) / dpr;
    m_height = static_cast<float>(size.height) / dpr;
    updateNodes();
    m_window->setResizeCallback([this](WindowSize new_size) {
        const float ratio = m_window->devicePixelRatio();
        m_renderer->resize({static_cast<std::uint32_t>(new_size.width), static_cast<std::uint32_t>(new_size.height)},
                           ratio);
        m_width = static_cast<float>(new_size.width) / ratio;
        m_height = static_cast<float>(new_size.height) / ratio;
        updateNodes();
    });
    context.add(m_renderer.get());
}

void Widget::setWindowTitle(const std::string& title) {
    m_window_title = title;
    if (isTopLevel() && m_window != nullptr) {
        m_window->setWindowTitle(title);
    }
}

void Widget::setGeometry(float x, float y, float width, float height) {
    m_x = x;
    m_y = y;
    if (!isTopLevel() || m_window == nullptr) {
        m_width = std::max(width, 0.0f);
        m_height = std::max(height, 0.0f);
    }
    updateNodes();
}

void Widget::setBackgroundColor(const Color& color) {
    if (m_background != nullptr) {
        m_background->setColor(toColorF(color));
    }
}

sg::Node* Widget::contentNode() const {
    return m_content;
}

void Widget::updateNodes() {
    if (m_node == nullptr) {
        return;
    }
    m_node->setMatrix(isTopLevel() ? sg::Transform2D{} : sg::Transform2D::translation(m_x, m_y));
    m_background->setRect({0.0f, 0.0f, m_width, m_height});
    m_clip->setRect({0.0f, 0.0f, m_width, m_height});
}
