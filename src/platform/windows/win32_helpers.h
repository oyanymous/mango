#pragma once

/// @file
/// Small Win32 helpers used by the Windows platform backend.

#include <windows.h>

#include <string>


/// Registers a window class.
/// @param hInstance Module that owns the class.
/// @param wndProc Window procedure for windows of this class.
/// @param className Class name; must stay valid while the class is registered.
/// @return The registered class description (registration failures are not reported yet).
WNDCLASSW createWindowClass(const HINSTANCE& hInstance, const WNDPROC& wndProc, const LPCWSTR& className);

/// Creates a hidden, overlapped top-level window with default size and position.
/// @param hInstance Module that registered @p className.
/// @param className A class registered with createWindowClass().
/// @param windowTitle Title bar text.
/// @return The new window, or null on failure.
HWND createWindow(const HINSTANCE& hInstance, const LPCWSTR& className, const LPCWSTR& windowTitle);

/// Converts a UTF-8 string to UTF-16 for Win32 wide-character APIs.
/// @param str UTF-8 text.
/// @return The UTF-16 text; empty for empty input.
std::wstring toWide(const std::string& str);
