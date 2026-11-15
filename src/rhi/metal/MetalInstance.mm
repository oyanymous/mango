#include "MetalInstance.h"

#include <utility>

#include "MetalAdapter.h"
#include "MetalSwapchain.h"

namespace rhi::metal {

MetalInstance::MetalInstance(InstanceDesc desc, std::shared_ptr<MetalDebugReporter> reporter)
    : m_desc(std::move(desc)), m_reporter(std::move(reporter)) {
}

AdapterList MetalInstance::enumerateAdapters() {
    if (m_adapters_enumerated) {
        return m_adapters;
    }
    m_adapters_enumerated = true;

    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    for (id<MTLDevice> device in devices) {
        if (auto adapter = MetalAdapter::create(m_reporter, device)) {
            m_adapters.push_back(std::move(*adapter));
        }
    }
    return m_adapters;
}

Result<SurfaceRef> MetalInstance::createSurface(SurfaceDesc desc) {
    return MetalSurface::create(std::move(desc));
}

} // namespace rhi::metal
