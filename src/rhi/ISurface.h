#pragma once

#include <string>
#include <variant>

#include "RHIFwd.h"

namespace rhi {

struct Win32WindowHandle {
    void* hinstance = nullptr;
    void* hwnd = nullptr;
};

struct MetalLayerHandle {
    void* layer = nullptr;
};

using NativeWindowHandle = std::variant<std::monostate, Win32WindowHandle, MetalLayerHandle>;

struct SurfaceDesc {
    NativeWindowHandle window;
    std::string label;
};

class ISurface {
public:
    virtual ~ISurface() = default;

    virtual SurfaceDesc getDesc() const = 0;
};

} // namespace rhi
