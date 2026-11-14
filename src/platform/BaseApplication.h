#pragma once

/**
 * @file
 * @brief The OS-independent part of the application.
 */

#include <functional>
#include <utility>
#include <vector>

#include "IWidget.h"


/**
 * @brief OS-independent application state: the list of top-level widgets and the event-loop interface.
 *
 * @details
 * Each OS backend derives from it through PrivateApplication.
 */
class BaseApplication {
public:
    virtual ~BaseApplication() = default;

    /**
     * @brief Adds a widget to the top-level list. Does not take ownership.
     *
     * @param widget Widget to add; must stay alive while it is registered.
     */
    void addTopLevelWidget(IWidget* widget) { m_top_level_widgets.push_back(widget); }

    /**
     * @brief Returns a copy of the registered top-level widgets, in registration order.
     */
    [[nodiscard]] std::vector<IWidget*> topLevelWidgets() { return m_top_level_widgets; }

    /**
     * @brief Sets a function the event loop calls once per iteration, after pending events are handled.
     *
     * @details
     * With a frame callback the loop polls instead of blocking, so the callback runs continuously (e.g. to
     * render). Pass an empty function to go back to a blocking loop.
     *
     * @param callback Called on the thread that runs the event loop.
     */
    void setFrameCallback(std::function<void()> callback) { m_frame_callback = std::move(callback); }

    /**
     * @brief Runs the native event loop. Blocks until the application quits.
     */
    virtual void run() = 0;

    /**
     * @brief Makes run() return once the current loop iteration is done. Call on the event-loop thread.
     */
    virtual void quit() = 0;
protected:
    std::function<void()> m_frame_callback; /**< Called once per loop iteration; may be empty. */
private:
    std::vector<IWidget*> m_top_level_widgets;
};
