#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "IDevice.h"
#include "RHIFwd.h"
#include "Result.h"

namespace rhi {

enum class AdapterType {
    Unknown,
    DiscreteGpu,
    IntegratedGpu,
    VirtualGpu,
    Cpu,
};

struct Limits {
    std::uint32_t max_texture_dimension_2d = 0;
    std::uint32_t max_texture_array_layers = 0;
    std::uint32_t max_bind_groups = 0;
    std::uint64_t max_uniform_buffer_range = 0;
    std::uint64_t max_storage_buffer_range = 0;
    std::uint32_t max_vertex_attributes = 0;
    std::uint32_t max_vertex_buffers = 0;
    std::uint32_t max_color_attachments = 0;
    std::array<std::uint32_t, 3> max_compute_workgroup_count{};
    std::array<std::uint32_t, 3> max_compute_workgroup_size{};
    std::uint32_t max_compute_invocations_per_workgroup = 0;
    float max_sampler_anisotropy = 1.0f;
};

struct Capabilities {
    std::string name;
    AdapterType type = AdapterType::Unknown;
    std::uint32_t vendor_id = 0;
    std::uint32_t device_id = 0;
    std::uint32_t api_version_major = 0;
    std::uint32_t api_version_minor = 0;
    std::uint32_t api_version_patch = 0;
    DeviceFeatures features; // the optional features this adapter supports
    Limits limits;
};

// A physical GPU.
class IAdapter {
public:
    virtual ~IAdapter() = default;

    virtual Capabilities getCapabilities() const = 0;
    virtual Result<DeviceRef> createDevice(DeviceDesc desc) = 0;
};

} // namespace rhi
