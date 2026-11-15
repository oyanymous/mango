#pragma once

#include "Backend.h"
#include "IInstance.h"
#include "RHIFwd.h"
#include "Result.h"

namespace rhi {

class RHIFactory {
public:
    Result<InstanceRef> createInstance(Backend backend, InstanceDesc desc);

    BackendList availableBackends() const;
};

} // namespace rhi
