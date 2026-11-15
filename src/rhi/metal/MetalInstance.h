#pragma once

#include <memory>

#include "IInstance.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalInstance final : public IInstance {
public:
    MetalInstance(InstanceDesc desc, std::shared_ptr<MetalDebugReporter> reporter);

    AdapterList enumerateAdapters() override;

    Result<SurfaceRef> createSurface(SurfaceDesc desc) override;
private:
    InstanceDesc m_desc;
    AdapterList m_adapters;
    bool m_adapters_enumerated = false;
    std::shared_ptr<MetalDebugReporter> m_reporter;
};

} // namespace rhi::metal
