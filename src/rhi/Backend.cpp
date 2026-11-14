/**
 * @file
 * @brief Backend names and the list of compiled-in backends.
 */

#include "Backend.h"

#include <array>

namespace rhi {

std::string_view toString(Backend backend) {
    switch (backend) {
    case Backend::Vulkan:
        return "Vulkan";
    case Backend::Metal:
        return "Metal";
    case Backend::GL:
        return "GL";
    }
    return "Unknown";
}

std::span<const Backend> compiledBackends() {
#if defined(MANGO_RHI_VULKAN)
    static constexpr std::array backends{Backend::Vulkan};
    return backends;
#else
    return {};
#endif
}

} // namespace rhi
