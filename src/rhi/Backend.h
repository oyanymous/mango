#pragma once

#include <span>
#include <string_view>

namespace rhi {

/// A GPU API the RHI can be implemented on.
enum class Backend {
    Vulkan, /**< Vulkan 1.2+ (MoltenVK on macOS). Implemented in rhi::vulkan. */
    Metal,  /**< Apple Metal (macOS 13+). Implemented in rhi::metal. */
    GL,     /**< OpenGL. Designed (see docs/diagrams), not implemented. */
};

/// Returns the backend's name, e.g. `"Vulkan"`.
std::string_view toString(Backend backend);

/**
 * The backends compiled into this build, in order: Vulkan when the `MANGO_RHI_VULKAN` CMake option is ON,
 * then Metal when `MANGO_RHI_METAL` is ON (Apple only).
 *
 * A compiled backend may still be unusable at run time (no driver); RHIFactory::availableBackends() checks that.
 */
std::span<const Backend> compiledBackends();

} // namespace rhi
