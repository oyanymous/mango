#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include "IAdapter.h"
#include "ISurface.h"
#include "RHIFwd.h"
#include "Result.h"

namespace rhi {

enum class DebugSeverity {
    Verbose,
    Info,
    Warning,
    Error,
};

struct DebugMessage {
    DebugSeverity severity = DebugSeverity::Info;
    std::string_view message;
};

using DebugCallback = std::function<void(const DebugMessage&)>;

struct InstanceDesc {
    std::string application_name = "Mango";
    std::uint32_t application_version = 0;

    bool enable_validation = false;

    DebugCallback debug_callback;
};

class IInstance {
public:
    virtual ~IInstance() = default;

    virtual AdapterList enumerateAdapters() = 0;

    virtual Result<SurfaceRef> createSurface(SurfaceDesc desc) = 0;
};

} // namespace rhi
