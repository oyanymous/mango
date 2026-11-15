#pragma once

#include "PrivateApplication.h"

#include "IWidget.h"

class Application {
public:
    Application();

    static void addTopLevelWidget(IWidget* widget) { PrivateApplication::instance().addTopLevelWidget(widget); }

    static void quit() { PrivateApplication::instance().quit(); }

    static void setFrameCallback(std::function<void()> callback, FrameMode mode = FrameMode::Continuous) {
        PrivateApplication::instance().setFrameCallback(std::move(callback), mode);
    }

    static void requestFrame() { PrivateApplication::instance().requestFrame(); }

    void run();
};
