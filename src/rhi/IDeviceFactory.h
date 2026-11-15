#pragma once

#include "IInstance.h"
#include "RHIFwd.h"
#include "Result.h"

namespace rhi {

class IDeviceFactory {
public:
    virtual ~IDeviceFactory() = default;

    virtual Result<InstanceRef> createInstance(InstanceDesc desc) = 0;
};

} // namespace rhi
