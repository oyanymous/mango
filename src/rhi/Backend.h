#pragma once

/// @file
/// rhi::Backend: the GPU APIs the RHI can run on.

#include <span>
#include <string_view>

namespace rhi {

/// A GPU API the RHI can be implemented on.
enum class Backend {
    Vulkan, ///< Vulkan 1.2+ (MoltenVK on macOS). Implemented in rhi::vulkan.
    Metal,  ///< Apple Metal. Designed (see docs/diagrams), not implemented.
    GL,     ///< OpenGL. Designed (see docs/diagrams), not implemented.
};

/// Returns the backend's name, e.g. `"Vulkan"`.
/// @param backend Backend to name.
std::string_view toString(Backend backend);

/// The backends compiled into this build, e.g. Vulkan when the `MANGO_RHI_VULKAN` CMake option is ON.
///
/// A compiled backend may still be unusable at run time (no driver); RHIFactory::availableBackends() checks that.
std::span<const Backend> compiledBackends();

} // namespace rhi
