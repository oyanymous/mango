#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "IWidget.h"

enum class FrameMode {
    Continuous,
    OnDemand,
};

class BaseApplication {
public:
    virtual ~BaseApplication() = default;

    void addTopLevelWidget(IWidget* widget) { m_top_level_widgets.push_back(widget); }

    [[nodiscard]] std::vector<IWidget*> topLevelWidgets() { return m_top_level_widgets; }

    void setFrameCallback(std::function<void()> callback, FrameMode mode = FrameMode::Continuous) {
        m_frame_callback = std::move(callback);
        m_frame_mode = mode;
    }

    void requestFrame() { m_frame_requested = true; }

    virtual void run() = 0;

    virtual void quit() = 0;
protected:
    [[nodiscard]] bool frameDue() const {
        return m_frame_callback && (m_frame_mode == FrameMode::Continuous || m_frame_requested);
    }

    void runFrameIfDue() {
        if (frameDue()) {
            m_frame_requested = false;
            m_frame_callback();
        }
    }

    std::function<void()> m_frame_callback;
    FrameMode m_frame_mode = FrameMode::Continuous;
    bool m_frame_requested = false;
private:
    std::vector<IWidget*> m_top_level_widgets;
};
