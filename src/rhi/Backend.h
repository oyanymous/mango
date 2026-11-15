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

std::span<const Backend> compiledBackends();

} // namespace rhi
