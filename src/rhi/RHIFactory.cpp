/**
 * @file
 * @brief RHIFactory: the compiled registry of backend factories.
 */

#include "RHIFactory.h"

#include <string>

#include "IDeviceFactory.h"

#if defined(MANGO_RHI_VULKAN)
#include "VulkanDeviceFactory.h"
#endif

#if defined(MANGO_RHI_METAL)
#include "MetalDeviceFactory.h"
#endif

namespace rhi {

namespace {

/// The compiled registry: returns the IDeviceFactory of a backend built into this binary, or null. Factories are
/// created once and shared.
DeviceFactoryRef factoryFor(Backend backend) {
    switch (backend) {
    case Backend::Vulkan:
#if defined(MANGO_RHI_VULKAN)
    {
        static const DeviceFactoryRef factory = std::make_shared<vulkan::VulkanDeviceFactory>();
        return factory;
    }
#else
        return nullptr;
#endif
    case Backend::Metal:
#if defined(MANGO_RHI_METAL)
    {
        static const DeviceFactoryRef factory = std::make_shared<metal::MetalDeviceFactory>();
        return factory;
    }
#else
        return nullptr;
#endif
    case Backend::GL:
        return nullptr;
    }
    return nullptr;
}

/// True if a compiled-in backend's driver can be loaded on this machine.
bool hasDriver(Backend backend) {
    switch (backend) {
    case Backend::Vulkan:
#if defined(MANGO_RHI_VULKAN)
        return vulkan::VulkanDeviceFactory::isLoaderAvailable();
#else
        return false;
#endif
    case Backend::Metal:
#if defined(MANGO_RHI_METAL)
        return metal::MetalDeviceFactory::isAvailable();
#else
        return false;
#endif
    case Backend::GL:
        return false;
    }
    return false;
}

} // namespace

Result<InstanceRef> RHIFactory::createInstance(Backend backend, InstanceDesc desc) {
    DeviceFactoryRef factory = factoryFor(backend);
    if (!factory) {
        return Error{ErrorCode::BackendUnavailable, std::string(toString(backend)) + " backend is not compiled in"};
    }
    return factory->createInstance(std::move(desc));
}

BackendList RHIFactory::availableBackends() const {
    BackendList backends;
    for (Backend backend : compiledBackends()) {
        if (hasDriver(backend)) {
            backends.push_back(backend);
        }
    }
    return backends;
}

} // namespace rhi
