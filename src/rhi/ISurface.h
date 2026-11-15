#pragma once

#include <string>
#include <variant>

#include "RHIFwd.h"

namespace rhi {

/// A Win32 window, as opaque pointers so the RHI needs no `<windows.h>`.
struct Win32WindowHandle {
    void* hinstance = nullptr; /**< The window's `HINSTANCE`. */
    void* hwnd = nullptr;      /**< The `HWND`. */
};

/// A macOS view's Metal layer, as an opaque pointer so the RHI needs no Objective-C.
struct MetalLayerHandle {
    void* layer = nullptr; /**< A `CAMetalLayer*`. */
};

/**
 * A native window handle. `std::monostate` means "no window".
 *
 * Native handles are passed as opaque pointers so the RHI does not depend on the platform layer or OS headers.
 */
using NativeWindowHandle = std::variant<std::monostate, Win32WindowHandle, MetalLayerHandle>;

/// Describes a surface to create with IInstance::createSurface.
struct SurfaceDesc {
    NativeWindowHandle window; /**< The window; must outlive the surface. */
    std::string label;
};

/// A window the GPU can present to. Swapchains are created on a surface.
class ISurface {
public:
    virtual ~ISurface() = default;

    virtual SurfaceDesc getDesc() const = 0;
};

} // namespace rhi
