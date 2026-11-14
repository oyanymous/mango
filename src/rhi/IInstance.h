#pragma once

#include <cstdint>
#include <string>

#include "IAdapter.h"
#include "ISurface.h"
#include "RHIFwd.h"
#include "Result.h"

namespace rhi {

struct InstanceDesc {
    std::string application_name = "Mango";
    std::uint32_t application_version = 0;
    bool enable_validation = false; // backend validation layers + debug messages
};

// Connection to one backend's driver; enumerates adapters and creates surfaces.
class IInstance {
public:
    virtual ~IInstance() = default;

    virtual AdapterList enumerateAdapters() = 0;
    virtual Result<SurfaceRef> createSurface(SurfaceDesc desc) = 0;
};

} // namespace rhi
