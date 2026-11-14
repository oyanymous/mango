#pragma once

/// @file
/// The basic UI element.

#include "../platform/Window.h"
#include "IWidget.h"


/// The basic UI element. A widget without a parent is a top-level widget with its own native Window.
///
/// Top-level widgets register themselves with the Application when constructed. The native window is created
/// lazily, on the first display().
class Widget : public IWidget {
public:
    /// Creates a widget.
    /// @param parent Parent widget, or null for a top-level widget. Not owned.
    explicit Widget(Widget* parent = nullptr);
    /// True if the widget has no parent and therefore gets its own window.
    [[nodiscard]] bool isTopLevel() const { return m_parent == nullptr; }
    /// Shows the widget, creating its native window on first call.
    void display() override;
    /// Sets the window title. Applied now if the window exists, otherwise when it is created.
    /// @param title UTF-8 title.
    void setWindowTitle(const std::string& title);
    /// The window title last set with setWindowTitle().
    [[nodiscard]] const std::string& getWindowTitle() const { return m_window_title; }
private:
    Widget* m_parent = nullptr;
    Window* m_window = nullptr;
    std::string m_window_title;

    [[nodiscard]] bool nativeWindowIsInitialized() const { return m_window != nullptr; }
};
