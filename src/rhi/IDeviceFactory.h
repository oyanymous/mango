#pragma once

#include "IInstance.h"
#include "RHIFwd.h"
#include "Result.h"

namespace rhi {

/// The entry point of one backend. RHIFactory keeps one per compiled backend; applications use RHIFactory.
class IDeviceFactory {
public:
    virtual ~IDeviceFactory() = default;

    /// Creates an instance of this backend.
    virtual Result<InstanceRef> createInstance(InstanceDesc desc) = 0;
};

} // namespace rhi
