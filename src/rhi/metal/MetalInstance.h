#pragma once

#include <memory>

#include "IInstance.h"
#include "MetalCommon.h"

namespace rhi::metal {

/// A Metal instance. Metal has no instance object of its own; this holds the debug reporter and the adapters.
class MetalInstance final : public IInstance {
public:
    /**
     * Creates an instance. Validation is set up by MetalDeviceFactory before this runs.
     *
     * @param reporter Debug reporter, shared with the adapters.
     */
    MetalInstance(InstanceDesc desc, std::shared_ptr<MetalDebugReporter> reporter);

    /// Wraps every device from `MTLCopyAllDevices()` that supports at least the Mac2 GPU family. Enumerated once, on
    /// the first call.
    AdapterList enumerateAdapters() override;

    /// Creates a MetalSurface for a MetalLayerHandle.
    Result<SurfaceRef> createSurface(SurfaceDesc desc) override;
private:
    InstanceDesc m_desc;
    AdapterList m_adapters; // enumerated once, on first use
    bool m_adapters_enumerated = false;
    std::shared_ptr<MetalDebugReporter> m_reporter;
};

} // namespace rhi::metal
