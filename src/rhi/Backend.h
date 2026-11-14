#pragma once

#include <span>
#include <string_view>

namespace rhi {

enum class Backend {
    Vulkan,
    Metal,
    GL,
};

std::string_view toString(Backend backend);

// Backends compiled into this build (e.g. Vulkan when MANGO_RHI_VULKAN is ON).
std::span<const Backend> compiledBackends();

} // namespace rhi
