#pragma once

#include "BaseApplication.h"

#include <memory>


/**
 * The process-wide application object that owns the native application state.
 *
 * A singleton (see instance()) so that widgets can register themselves without holding an Application.
 * Native state lives behind a pimpl (`Impl`), defined per OS in `darwin/PrivateApplication.mm` or
 * `windows/PrivateApplication.cpp`, so this header stays plain C++.
 */
class PrivateApplication : public BaseApplication {
public:
    PrivateApplication(const PrivateApplication&) = delete;
    PrivateApplication& operator=(const PrivateApplication&) = delete;
    PrivateApplication(PrivateApplication&&) = delete;
    PrivateApplication& operator=(PrivateApplication&&) = delete;

    /// Returns the singleton, creating it (and the native application state) on first call.
    static PrivateApplication& instance() {
        static PrivateApplication instance;
        return instance;
    }

    /// Runs the native event loop (Cocoa run loop / Win32 message loop). Blocks until the application quits.
    void run() override;

    /// Makes run() return (posts WM_QUIT on Win32; stops the hand-written loop on macOS).
    void quit() override;
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    PrivateApplication();
    ~PrivateApplication() override;
};
