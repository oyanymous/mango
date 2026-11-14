#pragma once

/**
 * @file
 * @brief A native top-level window.
 */

#include <functional>
#include <memory>

#include "BaseWindow.h"

/**
 * @brief The OS handles of a window, as opaque pointers so callers need no OS headers.
 */
struct NativeWindow {
    void* hinstance = nullptr;   /**< Windows: the `HINSTANCE` that owns the window. */
    void* hwnd = nullptr;        /**< Windows: the `HWND`. */
    void* metal_layer = nullptr; /**< macOS: the content view's `CAMetalLayer*`. */
};

/**
 * @brief Size of a window's client (content) area, in physical pixels. Zero while minimized.
 */
struct WindowSize {
    int width = 0;  /**< Width in pixels. */
    int height = 0; /**< Height in pixels. */
};

/**
 * @brief A native top-level window (NSWindow on macOS, HWND on Windows).
 *
 * @details
 * The native handles live behind a pimpl (`Impl`), defined per OS in `darwin/Window.mm` or
 * `windows/Window.cpp`, so no AppKit or Win32 types leak into C++ code that includes this header.
 * The window is created hidden; call display() to show it.
 */
class Window : public BaseWindow {
public:
    /**
     * @brief Creates the native window, hidden.
     *
     * @note On macOS creating a window initializes Metal, which fixes the process's Metal settings: create an RHI
     *       instance with validation before the first window, or Metal API validation stays off.
     *
     * @param window_title UTF-8 title for the title bar.
     */
    explicit Window(const std::string& window_title = "");
    ~Window() override;

    void display() override;
    void setWindowTitle(const std::string& title) override;

    /**
     * @brief The OS handles, e.g. to create a GPU surface. Valid until the window is destroyed.
     *
     * @note On macOS `metal_layer` is the content view's `CAMetalLayer`; its `drawableSize` is left to the renderer.
     */
    [[nodiscard]] NativeWindow nativeWindow() const;

    /**
     * @brief The current client-area size in pixels.
     */
    [[nodiscard]] WindowSize clientSize() const;

    /**
     * @brief Pixels per logical unit: 2 on a Retina display, the DPI / 96 on Windows, otherwise 1.
     */
    [[nodiscard]] float devicePixelRatio() const;

    /**
     * @brief Sets a function called whenever the client area changes size, including minimize (size 0).
     *
     * @note On macOS also called when the window moves to a screen with another scale factor.
     *
     * @param callback Called on the event-loop thread with the new size; may be empty.
     */
    void setResizeCallback(std::function<void(WindowSize)> callback);
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::string m_window_title;
};
