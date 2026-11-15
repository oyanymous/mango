#pragma once

#include "IDeviceFactory.h"

namespace rhi::metal {

class MetalDeviceFactory final : public IDeviceFactory {
public:
    static bool isAvailable();

    Result<InstanceRef> createInstance(InstanceDesc desc) override;
};

} // namespace rhi::metal
