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
    if (!m_frame_callback) {
        // Standard Win32 message pump; GetMessage returns 0 once WM_QUIT (posted on WM_DESTROY) arrives.
        while (GetMessage(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return;
    }
    // With a frame callback, drain pending messages without blocking, then run one frame.
    for (;;) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        m_frame_callback();
    }
}
