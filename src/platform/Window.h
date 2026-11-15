#pragma once

#include <functional>
#include <memory>

#include "BaseWindow.h"

struct NativeWindow {
    void* hinstance = nullptr;
    void* hwnd = nullptr;
    void* metal_layer = nullptr;
};

struct WindowSize {
    int width = 0;
    int height = 0;
};

class Window : public BaseWindow {
public:
    explicit Window(const std::string& window_title = "");
    ~Window() override;

    void display() override;
    void setWindowTitle(const std::string& title) override;

    [[nodiscard]] NativeWindow nativeWindow() const;

    [[nodiscard]] WindowSize clientSize() const;

    [[nodiscard]] float devicePixelRatio() const;

    void setResizeCallback(std::function<void(WindowSize)> callback);
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::string m_window_title;
};
