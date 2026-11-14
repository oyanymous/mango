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

void PrivateApplication::run() {
    // Standard Win32 message pump; GetMessage returns 0 once WM_QUIT (posted on WM_DESTROY) arrives.
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
