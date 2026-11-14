#pragma once

/**
 * @file
 * @brief Public entry point of the platform layer: the application object.
 */

#include "PrivateApplication.h"

#include "IWidget.h"


/**
 * @brief The application: sets up the native application state and runs the event loop.
 *
 * @details
 * A thin facade over the PrivateApplication singleton. Construct exactly one, before creating any Window or
 * Widget (on macOS this initializes `[NSApplication sharedApplication]`).
 *
 * @code
 * Application app;
 * Widget widget;
 * widget.display();
 * app.run(); // blocks until the last window closes
 * @endcode
 */
class Application {
public:
    /**
     * @brief Initializes the native application state.
     */
    Application();

    /**
     * @brief Registers a top-level widget with the application.
     *
     * @details
     * Called by Widget for every widget without a parent. The application does not take ownership.
     *
     * @param widget Widget to register; must outlive the application's event loop.
     */
    static void addTopLevelWidget(IWidget* widget) { PrivateApplication::instance().addTopLevelWidget(widget); }

    /**
     * @brief Sets a function the event loop calls once per iteration; see BaseApplication::setFrameCallback.
     *
     * @note On macOS the callback does not run while the user drags the window edge (AppKit runs its own loop for
     *       live resizing); it resumes, and the resize callback fires, when the drag ends.
     *
     * @param callback Called on the event-loop thread; an empty function restores the blocking loop.
     */
    static void setFrameCallback(std::function<void()> callback) {
        PrivateApplication::instance().setFrameCallback(std::move(callback));
    }

    /**
     * @brief Runs the native event loop. Blocks until the application quits, i.e. until a window is closed.
     */
    void run();
};
