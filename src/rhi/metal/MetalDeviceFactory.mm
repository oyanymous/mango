#include "MetalDeviceFactory.h"

#include <cstdlib>
#include <memory>
#include <utility>

#include "MetalCommon.h"
#include "MetalInstance.h"

#if !__has_feature(objc_arc)
#error "The Metal backend must be compiled with ARC (-fobjc-arc)"
#endif

namespace rhi::metal {

namespace {

void enableValidationLayer() {
    setenv("MTL_DEBUG_LAYER", "1", 0);
    setenv("MTL_DEBUG_LAYER_ERROR_MODE", "assert", 0);
    setenv("MTL_DEBUG_LAYER_WARNING_MODE", "assert", 0);
    setenv("MTL_SHADER_VALIDATION", "1", 0);
}

bool isValidated(id<MTLDevice> device) {
    return [NSStringFromClass([device class]) containsString:@"Debug"];
}

} // namespace

bool MetalDeviceFactory::isAvailable() {
    return true;
}

Result<InstanceRef> MetalDeviceFactory::createInstance(InstanceDesc desc) {
    auto reporter = std::make_shared<MetalDebugReporter>(desc.debug_callback);

    if (desc.enable_validation) {
        enableValidationLayer();
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (device != nil && isValidated(device)) {
            reporter->report(DebugSeverity::Info, "using Metal API validation");
        } else if (device != nil) {
            reporter->report(
                DebugSeverity::Warning,
                "validation requested but the Metal API validation layer is not active (Metal was used "
                "before the instance was created, or MTL_DEBUG_LAYER / MTL_SHADER_VALIDATION is set to 0)");
        }
    }

    return InstanceRef(std::make_shared<MetalInstance>(std::move(desc), std::move(reporter)));
}

} // namespace rhi::metal
