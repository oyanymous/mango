/**
 * @file
 * @brief MetalDeviceFactory: Metal instance creation.
 */

#include "MetalDeviceFactory.h"

// The backend relies on ARC for every Metal object it holds; src/rhi/CMakeLists.txt sets -fobjc-arc.
#if !__has_feature(objc_arc)
#error "The Metal backend must be compiled with ARC (-fobjc-arc)"
#endif

namespace rhi::metal {

Result<InstanceRef> MetalDeviceFactory::createInstance(InstanceDesc) {
    return Error{ErrorCode::Unsupported, "the Metal backend cannot create instances yet"};
}

} // namespace rhi::metal
