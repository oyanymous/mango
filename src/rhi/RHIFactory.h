#pragma once

/// @file
/// rhi::RHIFactory: where an application starts using the RHI.

#include "Backend.h"
#include "IInstance.h"
#include "RHIFwd.h"
#include "Result.h"

namespace rhi {

/// Where an application starts: lists usable backends and creates instances of them.
///
/// Holds a compiled registry of one IDeviceFactory per backend built into this binary. Cheap to construct.
/// @code
/// rhi::RHIFactory factory;
/// auto instance = factory.createInstance(rhi::Backend::Vulkan, {.enable_validation = true});
/// @endcode
class RHIFactory {
public:
    /// Creates an instance of a backend.
    /// @param backend Backend to use.
    /// @param desc Application info, validation and debug callback.
    /// @return The instance, or ErrorCode::BackendUnavailable if the backend is not compiled in or its
    ///         driver is missing.
    Result<InstanceRef> createInstance(Backend backend, InstanceDesc desc);

    /// The compiled-in backends whose driver is present on this machine.
    BackendList availableBackends() const;
};

} // namespace rhi
