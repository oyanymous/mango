#pragma once

/**
 * @file
 * @brief The widget interface shared by the platform and UI layers.
 */

/**
 * @brief Something that can be shown on screen as (or inside) a native window.
 *
 * @details
 * Lives in `Core` so the platform layer can keep a list of top-level widgets without depending on the UI module.
 */
class IWidget {
public:
    virtual ~IWidget() = default;

    /**
     * @brief Shows the widget, creating its native window on first use.
     */
    virtual void display() = 0;
};
