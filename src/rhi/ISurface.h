#pragma once

#include <string>
#include <variant>

#include "RHIFwd.h"

namespace rhi {

// Native window handles are passed as opaque pointers so RHI does not depend on Platform or OS headers.
struct Win32WindowHandle {
    void* hinstance = nullptr; // HINSTANCE
    void* hwnd = nullptr;      // HWND
};

struct MetalLayerHandle {
    void* layer = nullptr; // CAMetalLayer*
};

using NativeWindowHandle = std::variant<std::monostate, Win32WindowHandle, MetalLayerHandle>;

struct SurfaceDesc {
    NativeWindowHandle window;
    std::string label;
};

// A window the GPU can present to.
class ISurface {
public:
    virtual ~ISurface() = default;

    virtual SurfaceDesc getDesc() const = 0;
};

} // namespace rhi
