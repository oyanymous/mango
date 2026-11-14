#pragma once

/// @file
/// The widget interface shared by the platform and UI layers.

/// Something that can be shown on screen as (or inside) a native window.
///
/// Lives in `Core` so the platform layer can keep a list of top-level widgets without depending on the UI module.
class IWidget {
public:
    virtual ~IWidget() = default;

    /// Shows the widget, creating its native window on first use.
    virtual void display() = 0;
};
