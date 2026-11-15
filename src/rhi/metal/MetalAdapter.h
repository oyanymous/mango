#pragma once

#include <memory>
#include <optional>

#include "IAdapter.h"
#include "MetalCommon.h"

namespace rhi::metal {

/**
 * A Metal GPU (`id<MTLDevice>`).
 *
 * Metal exposes no PCI device ID, so Capabilities::device_id is 0. Capabilities::vendor_id is Apple's for Apple
 * GPUs and is guessed from the device name otherwise (AMD, Intel, NVIDIA). The API version is the newest Metal
 * GPU family the device supports (e.g. 3.0 for MTLGPUFamilyMetal3).
 */
class MetalAdapter final : public IAdapter {
public:
    /**
     * Queries a device and wraps it if the backend can use it.
     *
     * @param device Device from `MTLCopyAllDevices()`.
     * @return The adapter, or nothing for a GPU outside the Mac2 family.
     */
    static std::optional<AdapterRef> create(std::shared_ptr<MetalDebugReporter> reporter, id<MTLDevice> device);

    /// Wraps a device whose capabilities were already queried. Use create().
    MetalAdapter(std::shared_ptr<MetalDebugReporter> reporter, id<MTLDevice> device, Capabilities capabilities);

    Capabilities getCapabilities() const override;

    /// Creates a MetalDevice on this GPU, with one command queue per QueueType.
    Result<DeviceRef> createDevice(DeviceDesc desc) override;
private:
    Capabilities m_capabilities;
    id<MTLDevice> m_device = nil;
    std::shared_ptr<MetalDebugReporter> m_reporter;
};

} // namespace rhi::metal
