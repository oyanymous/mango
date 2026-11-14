#pragma once

/// @file
/// A native top-level window.

#include <memory>

#include "BaseWindow.h"

/// A native top-level window (NSWindow on macOS, HWND on Windows).
///
/// The native handles live behind a pimpl (`Impl`), defined per OS in `darwin/Window.mm` or
/// `windows/Window.cpp`, so no AppKit or Win32 types leak into C++ code that includes this header.
/// The window is created hidden; call display() to show it.
class Window : public BaseWindow {
public:
    /// Creates the native window, hidden.
    /// @param window_title UTF-8 title for the title bar.
    explicit Window(const std::string& window_title = "");
    ~Window() override;

    void display() override;
    void setWindowTitle(const std::string& title) override;
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::string m_window_title;
};
