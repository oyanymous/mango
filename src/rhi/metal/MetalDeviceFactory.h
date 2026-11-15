#pragma once

#include "IDeviceFactory.h"

namespace rhi::metal {

/**
 * Creates Metal instances. Registered in RHIFactory for Backend::Metal. Plain C++, so RHIFactory.cpp can
 * include it.
 *
 * With InstanceDesc::enable_validation, createInstance() turns on Metal API and shader validation through the
 * `MTL_DEBUG_LAYER` / `MTL_SHADER_VALIDATION` environment variables. Metal reads them only when the process creates
 * its first device, so create the instance before anything else uses Metal; otherwise a DebugSeverity::Warning is
 * reported and the instance runs without validation. Validation errors and warnings abort the program.
 */
class MetalDeviceFactory final : public IDeviceFactory {
public:
    /// True on every macOS the build supports: Metal is part of the OS. Does not touch Metal, so validation can still
    /// be enabled afterwards. A machine without a GPU gets an instance with no adapters.
    static bool isAvailable();

    Result<InstanceRef> createInstance(InstanceDesc desc) override;
};

} // namespace rhi::metal
