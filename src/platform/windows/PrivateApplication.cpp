/**
 * @file
 * @brief Win32 implementation of PrivateApplication: the message loop.
 */

#include "../PrivateApplication.h"

#include <windows.h>

/**
 * @brief Win32 needs no per-application state yet.
 */
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
            // Nothing to render: block for the next message. GetMessage returns 0 once WM_QUIT (posted on
            // WM_DESTROY) arrives.
            if (GetMessage(&msg, NULL, 0, 0) <= 0) {
                return;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Drain pending messages without blocking, then run the frame if one is due.
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
