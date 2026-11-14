#pragma once

/// @file
/// rhi::IAdapter: a physical GPU, its capabilities, and device creation.

#include <array>
#include <cstdint>
#include <string>

#include "IDevice.h"
#include "RHIFwd.h"
#include "Result.h"

namespace rhi {

/// The kind of GPU.
enum class AdapterType {
    Unknown,       ///< Not reported.
    DiscreteGpu,   ///< A separate graphics card.
    IntegratedGpu, ///< A GPU sharing memory with the CPU.
    VirtualGpu,    ///< A GPU in a virtual machine.
    Cpu,           ///< A software rasterizer.
};

/// Hard limits of an adapter. Exceeding one is invalid usage.
struct Limits {
    std::uint32_t max_texture_dimension_2d = 0;                 ///< Largest width/height of a 2D texture.
    std::uint32_t max_texture_array_layers = 0;                 ///< Most array layers in a texture.
    std::uint32_t max_bind_groups = 0;                          ///< Most bind groups in a pipeline layout.
    std::uint64_t max_uniform_buffer_range = 0;                 ///< Largest uniform buffer binding, in bytes.
    std::uint64_t max_storage_buffer_range = 0;                 ///< Largest storage buffer binding, in bytes.
    std::uint32_t max_vertex_attributes = 0;                    ///< Most vertex attributes in a pipeline.
    std::uint32_t max_vertex_buffers = 0;                       ///< Most vertex buffer slots in a pipeline.
    std::uint32_t max_color_attachments = 0;                    ///< Most colour attachments in a render pass.
    std::array<std::uint32_t, 3> max_compute_workgroup_count{}; ///< Most workgroups per dispatch, per axis.
    std::array<std::uint32_t, 3> max_compute_workgroup_size{};  ///< Largest workgroup size, per axis.
    std::uint32_t max_compute_invocations_per_workgroup = 0;    ///< Largest workgroup size, in total.
    float max_sampler_anisotropy = 1.0f;                        ///< Largest SamplerDesc::max_anisotropy.
};

/// What an adapter is and what it supports.
struct Capabilities {
    std::string name;                        ///< Driver-reported GPU name.
    AdapterType type = AdapterType::Unknown; ///< Kind of GPU.
    std::uint32_t vendor_id = 0;             ///< PCI vendor ID (e.g. 0x10DE NVIDIA, 0x1002 AMD, 0x8086 Intel).
    std::uint32_t device_id = 0;             ///< PCI device ID.
    std::uint32_t api_version_major = 0;     ///< Highest backend API version supported (e.g. Vulkan 1.3).
    std::uint32_t api_version_minor = 0;     ///< Minor part of the API version.
    std::uint32_t api_version_patch = 0;     ///< Patch part of the API version.
    DeviceFeatures features;                 ///< Optional features that can be enabled in DeviceDesc.
    Limits limits;                           ///< Hard limits.
};

/// A physical GPU. Obtained from IInstance::enumerateAdapters.
///
/// Keeps its instance's native state alive, so it stays valid after the InstanceRef is dropped.
class IAdapter {
public:
    virtual ~IAdapter() = default;

    /// The adapter's name, type, features and limits.
    virtual Capabilities getCapabilities() const = 0;

    /// Creates a logical device on this adapter.
    /// @param desc Features to enable.
    /// @return The device, or ErrorCode::Unsupported if a requested feature is unavailable.
    virtual Result<DeviceRef> createDevice(DeviceDesc desc) = 0;
};

} // namespace rhi
