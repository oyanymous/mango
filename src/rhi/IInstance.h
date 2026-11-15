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
    Verbose, /**< Diagnostic chatter. */
    Info,    /**< Informational, e.g. which validation layer is in use. */
    Warning, /**< Possible misuse or a performance problem. */
    Error,   /**< Invalid API usage reported by validation; a bug to fix. */
};

/// A message from the backend or its validation layer.
struct DebugMessage {
    DebugSeverity severity = DebugSeverity::Info; /**< How serious it is. */
    std::string_view message;                     /**< The text; valid only during the callback. */
};

/// Receives DebugMessage values. May be called from any thread the backend uses, for as long as the instance (or
/// anything created from it) lives.
using DebugCallback = std::function<void(const DebugMessage&)>;

/// Describes an instance to create with RHIFactory::createInstance.
struct InstanceDesc {
    std::string application_name = "Mango"; /**< Reported to the driver. */
    std::uint32_t application_version = 0;  /**< Reported to the driver. */
    /// Enables the backend's validation layer. If it cannot be found, the instance is still created and a
    /// DebugSeverity::Warning is reported through `debug_callback`.
    bool enable_validation = false;
    /// Receives warnings, errors and validation messages. Null prints them to stderr.
    DebugCallback debug_callback;
};

/**
 * A connection to one backend's driver: enumerates adapters and creates surfaces.
 *
 * Created by RHIFactory::createInstance.
 */
class IInstance {
public:
    virtual ~IInstance() = default;

    /**
     * Lists the GPUs this backend can use. Enumerated on first call, then cached.
     *
     * GPUs the backend cannot run on (e.g. below Vulkan 1.2) are left out.
     */
    virtual AdapterList enumerateAdapters() = 0;

    /// Creates a surface for a native window.
    virtual Result<SurfaceRef> createSurface(SurfaceDesc desc) = 0;
};

} // namespace rhi
