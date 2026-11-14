#pragma once

#include "Backend.h"
#include "IInstance.h"
#include "RHIFwd.h"
#include "Result.h"

namespace rhi {

// Picks a backend from the ones compiled into this build (a registry of IDeviceFactory, one per backend).
// Implemented in step 3 of docs/RHI_VULKAN_PLAN.md.
class RHIFactory {
public:
    // Fails with ErrorCode::BackendUnavailable if the backend is not compiled in or has no driver.
    Result<InstanceRef> createInstance(Backend backend, InstanceDesc desc);
    BackendList availableBackends() const;
};

} // namespace rhi
