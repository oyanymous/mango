#pragma once

#include <memory>
#include <optional>

#include "IAdapter.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalAdapter final : public IAdapter {
public:
    static std::optional<AdapterRef> create(std::shared_ptr<MetalDebugReporter> reporter, id<MTLDevice> device);

    MetalAdapter(std::shared_ptr<MetalDebugReporter> reporter, id<MTLDevice> device, Capabilities capabilities);

    Capabilities getCapabilities() const override;

    Result<DeviceRef> createDevice(DeviceDesc desc) override;
private:
    Capabilities m_capabilities;
    id<MTLDevice> m_device = nil;
    std::shared_ptr<MetalDebugReporter> m_reporter;
};

} // namespace rhi::metal
