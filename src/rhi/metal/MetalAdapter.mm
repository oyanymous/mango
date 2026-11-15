/**
 * @file
 * @brief MetalAdapter: capability queries for a Metal GPU.
 */

#include "MetalAdapter.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

#include "MetalDevice.h"

namespace rhi::metal {

namespace {

/// Metal API version of a device: the newest `MTLGPUFamilyMetalN` it supports, else 2 (every Mac2 GPU).
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

/// PCI vendor ID: Apple for Apple-family GPUs; otherwise guessed from the name, since Metal does not report it.
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

/// Limits of a Mac2-family GPU. Most are fixed by Apple's Metal feature set tables, not queryable.
Limits limits(id<MTLDevice> device) {
    const MTLSize threads = device.maxThreadsPerThreadgroup;
    return Limits{
        .max_texture_dimension_2d = 16384,
        .max_texture_array_layers = 2048,
        // Bind groups are flattened into Metal's per-stage argument tables; 4 matches Vulkan's guaranteed minimum.
        .max_bind_groups = 4,
        // Metal documents no cap on a constant buffer binding; 64 KiB keeps shaders portable to Vulkan.
        .max_uniform_buffer_range = 64 * 1024,
        .max_storage_buffer_range = device.maxBufferLength,
        .max_vertex_attributes = 31,
        // Vertex buffers share the 31 buffer slots with bind groups; the top 8 slots are reserved for them.
        .max_vertex_buffers = 8,
        .max_color_attachments = 8,
        // Metal documents no lower limit per dispatch axis; Vulkan's guaranteed minimum keeps code portable.
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
    // Every Mac2-family GPU supports anisotropic filtering and wireframe fill.
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
