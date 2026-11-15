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
#if defined(MANGO_RHI_VULKAN) && defined(MANGO_RHI_METAL)
    static constexpr std::array backends{Backend::Vulkan, Backend::Metal};
    return backends;
#elif defined(MANGO_RHI_VULKAN)
    static constexpr std::array backends{Backend::Vulkan};
    return backends;
#elif defined(MANGO_RHI_METAL)
    static constexpr std::array backends{Backend::Metal};
    return backends;
#else
    return {};
#endif
}

} // namespace rhi
