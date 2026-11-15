#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "IWidget.h"


/// When the event loop calls the frame callback.
enum class FrameMode {
    Continuous, /**< Every loop iteration; the loop polls instead of blocking (games, benchmarks). */
    OnDemand,   /**< Only after BaseApplication::requestFrame(); the loop sleeps until an event or a request. */
};

/**
 * OS-independent application state: the list of top-level widgets and the event-loop interface.
 *
 * Each OS backend derives from it through PrivateApplication.
 */
class BaseApplication {
public:
    virtual ~BaseApplication() = default;

    /**
     * Adds a widget to the top-level list. Does not take ownership.
     *
     * @param widget Widget to add; must stay alive while it is registered.
     */
    void addTopLevelWidget(IWidget* widget) { m_top_level_widgets.push_back(widget); }

    /// Returns a copy of the registered top-level widgets, in registration order.
    [[nodiscard]] std::vector<IWidget*> topLevelWidgets() { return m_top_level_widgets; }

    /**
     * Sets a function the event loop calls after pending events are handled.
     *
     * In FrameMode::Continuous the loop polls instead of blocking, so the callback runs every iteration (e.g. to
     * render). In FrameMode::OnDemand it runs once per requestFrame() (requests before it runs coalesce), and the
     * loop blocks for events in between, so an idle application uses no CPU. Pass an empty function to go back to a
     * blocking loop without callback.
     *
     * @param callback Called on the thread that runs the event loop.
     * @param mode When the callback runs.
     */
    void setFrameCallback(std::function<void()> callback, FrameMode mode = FrameMode::Continuous) {
        m_frame_callback = std::move(callback);
        m_frame_mode = mode;
    }

    /**
     * Asks for one call of the frame callback in FrameMode::OnDemand (no effect in Continuous mode).
     *
     * Call on the event-loop thread, e.g. from an event handler or from the frame callback itself (to animate).
     * The callback runs at the end of the current loop iteration, after pending events.
     */
    void requestFrame() { m_frame_requested = true; }

    /// Runs the native event loop. Blocks until the application quits.
    virtual void run() = 0;

    /// Makes run() return once the current loop iteration is done. Call on the event-loop thread.
    virtual void quit() = 0;
protected:
    /// Whether the frame callback should run at the end of this loop iteration.
    [[nodiscard]] bool frameDue() const {
        return m_frame_callback && (m_frame_mode == FrameMode::Continuous || m_frame_requested);
    }

    /// Runs the frame callback if it is due, clearing the pending request first (so it may request again).
    void runFrameIfDue() {
        if (frameDue()) {
            m_frame_requested = false;
            m_frame_callback();
        }
    }

    std::function<void()> m_frame_callback;         /**< Called after events; may be empty. */
    FrameMode m_frame_mode = FrameMode::Continuous; /**< When m_frame_callback runs. */
    bool m_frame_requested = false;                 /**< A requestFrame() is pending (OnDemand). */
private:
    std::vector<IWidget*> m_top_level_widgets;
};
