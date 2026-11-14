#pragma once

/// @file
/// The OS-independent part of the application.

#include <vector>

#include "IWidget.h"


/// OS-independent application state: the list of top-level widgets and the event-loop interface.
///
/// Each OS backend derives from it through PrivateApplication.
class BaseApplication {
public:
    virtual ~BaseApplication() = default;

    /// Adds a widget to the top-level list. Does not take ownership.
    /// @param widget Widget to add; must stay alive while it is registered.
    void addTopLevelWidget(IWidget* widget) { m_top_level_widgets.push_back(widget); }

    /// Returns a copy of the registered top-level widgets, in registration order.
    [[nodiscard]] std::vector<IWidget*> topLevelWidgets() { return m_top_level_widgets; }

    /// Runs the native event loop. Blocks until the application quits.
    virtual void run() = 0;
private:
    std::vector<IWidget*> m_top_level_widgets;
};
