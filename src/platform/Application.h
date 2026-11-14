#pragma once

/// @file
/// Public entry point of the platform layer: the application object.

#include "PrivateApplication.h"

#include "IWidget.h"


/// The application: sets up the native application state and runs the event loop.
///
/// A thin facade over the PrivateApplication singleton. Construct exactly one, before creating any Window or
/// Widget (on macOS this initializes `[NSApplication sharedApplication]`).
///
/// @code
/// Application app;
/// Widget widget;
/// widget.display();
/// app.run(); // blocks until the last window closes
/// @endcode
class Application {
public:
    /// Initializes the native application state.
    Application();

    /// Registers a top-level widget with the application.
    ///
    /// Called by Widget for every widget without a parent. The application does not take ownership.
    /// @param widget Widget to register; must outlive the application's event loop.
    static void addTopLevelWidget(IWidget* widget) { PrivateApplication::instance().addTopLevelWidget(widget); }

    /// Runs the native event loop. Blocks until the application quits.
    void run();
};
