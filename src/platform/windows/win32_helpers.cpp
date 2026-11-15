#include "win32_helpers.h"

WNDCLASSW createWindowClass(const HINSTANCE& hInstance, const WNDPROC& wndProc, const LPCWSTR& className) {
    WNDCLASSW wc = {};

    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;

    RegisterClassW(&wc);
    return wc;
}

HWND createWindow(const HINSTANCE& hInstance, const LPCWSTR& className, const LPCWSTR& windowTitle) {
    HWND hwnd = CreateWindowExW(0, className, windowTitle, WS_OVERLAPPEDWINDOW,

                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

                                NULL, NULL, hInstance, NULL);
    return hwnd;
}

std::wstring toWide(const std::string& str) {
    if (str.empty())
        return {};

    const int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);

    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), size);
    return result;
}
