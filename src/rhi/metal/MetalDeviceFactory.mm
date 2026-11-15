/**
 * @file
 * @brief MetalDeviceFactory: Metal instance creation and API validation setup.
 */

#include "MetalDeviceFactory.h"

#include <cstdlib>
#include <memory>
#include <utility>

#include "MetalCommon.h"
#include "MetalInstance.h"

// The backend relies on ARC for every Metal object it holds; src/rhi/CMakeLists.txt sets -fobjc-arc.
#if !__has_feature(objc_arc)
#error "The Metal backend must be compiled with ARC (-fobjc-arc)"
#endif

namespace rhi::metal {

namespace {

/// Turns on Metal API and shader validation for this process. Metal reads these variables when it creates its first
/// device, so they only work if nothing in the process has used Metal yet. Values the developer already set win.
/// Warnings assert like errors, so any validation message stops the program (as it fails the Vulkan smoke test).
void enableValidationLayer() {
    setenv("MTL_DEBUG_LAYER", "1", 0);
    setenv("MTL_DEBUG_LAYER_ERROR_MODE", "assert", 0);
    setenv("MTL_DEBUG_LAYER_WARNING_MODE", "assert", 0);
    setenv("MTL_SHADER_VALIDATION", "1", 0);
}

/// True if @p device is wrapped by the API validation layer (its class is then `MTLDebugDevice`).
bool isValidated(id<MTLDevice> device) {
    return [NSStringFromClass([device class]) containsString:@"Debug"];
}

} // namespace

bool MetalDeviceFactory::isAvailable() {
    // Metal ships with every macOS the build targets (13+). Checking for GPUs here would create a Metal device and
    // lock in the validation settings before createInstance() can set them; a GPU-less machine (a VM) instead gets an
    // instance with no adapters.
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
