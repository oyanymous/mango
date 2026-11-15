#include "../PrivateApplication.h"

#include <windows.h>

struct PrivateApplication::Impl {};

PrivateApplication::PrivateApplication() : m_impl(std::make_unique<Impl>()) {
}

PrivateApplication::~PrivateApplication() = default;

void PrivateApplication::quit() {
    PostQuitMessage(0);
}

void PrivateApplication::run() {
    MSG msg = {};
    for (;;) {
        if (!frameDue()) {
            if (GetMessage(&msg, NULL, 0, 0) <= 0) {
                return;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        runFrameIfDue();
    }
}
