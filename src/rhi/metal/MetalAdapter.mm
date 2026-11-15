#include "MetalAdapter.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

#include "MetalDevice.h"

namespace rhi::metal {

namespace {

void apiVersion(id<MTLDevice> device, Capabilities& caps) {
    caps.api_version_major = 2;
#if defined(MAC_OS_VERSION_26_0)
    if (@available(macOS 26.0, *)) {
        if ([device supportsFamily:MTLGPUFamilyMetal4]) {
            caps.api_version_major = 4;
            return;
        }
    }
#endif
    if ([device supportsFamily:MTLGPUFamilyMetal3]) {
        caps.api_version_major = 3;
    }
}

std::uint32_t vendorId(id<MTLDevice> device, const std::string& name) {
    if ([device supportsFamily:MTLGPUFamilyApple1]) {
        return kAppleVendorId;
    }
    std::string lower = name;
    std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower.find("amd") != std::string::npos || lower.find("radeon") != std::string::npos) {
        return 0x1002;
    }
    if (lower.find("intel") != std::string::npos) {
        return 0x8086;
    }
    if (lower.find("nvidia") != std::string::npos) {
        return 0x10DE;
    }
    return 0;
}

Limits limits(id<MTLDevice> device) {
    const MTLSize threads = device.maxThreadsPerThreadgroup;
    return Limits{
        .max_texture_dimension_2d = 16384,
        .max_texture_array_layers = 2048,

        .max_bind_groups = 4,

        .max_uniform_buffer_range = 64 * 1024,
        .max_storage_buffer_range = device.maxBufferLength,
        .max_vertex_attributes = 31,

        .max_vertex_buffers = 8,
        .max_color_attachments = 8,

        .max_compute_workgroup_count = {65535, 65535, 65535},
        .max_compute_workgroup_size = {static_cast<std::uint32_t>(threads.width),
                                       static_cast<std::uint32_t>(threads.height),
                                       static_cast<std::uint32_t>(threads.depth)},
        .max_compute_invocations_per_workgroup = 1024,
        .max_sampler_anisotropy = 16.0f,
    };
}

} // namespace

std::optional<AdapterRef> MetalAdapter::create(std::shared_ptr<MetalDebugReporter> reporter, id<MTLDevice> device) {
    if (![device supportsFamily:MTLGPUFamilyMac2]) {
        return std::nullopt;
    }

    Capabilities caps;
    caps.name = device.name.UTF8String;
    caps.type = device.hasUnifiedMemory ? AdapterType::IntegratedGpu : AdapterType::DiscreteGpu;
    caps.vendor_id = vendorId(device, caps.name);
    apiVersion(device, caps);

    caps.features = {.sampler_anisotropy = true, .fill_mode_wireframe = true};
    caps.limits = limits(device);

    return AdapterRef(std::make_shared<MetalAdapter>(std::move(reporter), device, std::move(caps)));
}

MetalAdapter::MetalAdapter(std::shared_ptr<MetalDebugReporter> reporter, id<MTLDevice> device,
                           Capabilities capabilities)
    : m_capabilities(std::move(capabilities)), m_device(device), m_reporter(std::move(reporter)) {
}

Capabilities MetalAdapter::getCapabilities() const {
    return m_capabilities;
}

Result<DeviceRef> MetalAdapter::createDevice(DeviceDesc desc) {
    const DeviceFeatures& supported = m_capabilities.features;
    if ((desc.features.sampler_anisotropy && !supported.sampler_anisotropy) ||
        (desc.features.fill_mode_wireframe && !supported.fill_mode_wireframe)) {
        return Error{ErrorCode::Unsupported, "MetalAdapter::createDevice: a requested feature is not supported"};
    }
    auto context = std::make_shared<MetalDeviceContext>(MetalDeviceContext{m_device, m_reporter});
    return MetalDevice::create(std::move(context), std::move(desc));
}

} // namespace rhi::metal
