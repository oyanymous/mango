#pragma once

/**
 * @file
 * @brief rhi::IDeviceFactory: the entry point every backend implements.
 */

#include "IInstance.h"
#include "RHIFwd.h"
#include "Result.h"

namespace rhi {

/**
 * @brief The entry point of one backend. RHIFactory keeps one per compiled backend; applications use RHIFactory.
 */
class IDeviceFactory {
public:
    virtual ~IDeviceFactory() = default;

    /**
     * @brief Creates an instance of this backend.
     *
     * @param desc Application info, validation and debug callback.
     */
    virtual Result<InstanceRef> createInstance(InstanceDesc desc) = 0;
};

} // namespace rhi
