#pragma once

/// @file
/// rhi::IBindGroup: a set of resources bound to shaders together.

#include <cstdint>
#include <string>
#include <vector>

#include "IBindGroupLayout.h"
#include "IBuffer.h"
#include "ISampler.h"
#include "ITextureView.h"
#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

/// One resource bound to a slot of a bind group.
///
/// Set the members that match the layout entry's BindingType: `buffer` (with `offset` and `size`) for
/// buffers, `texture_view` for textures, `sampler` for samplers.
struct BindGroupEntry {
    std::uint32_t binding = 0;       ///< Slot number in the layout.
    std::uint32_t array_element = 0; ///< Element of an array slot.
    BufferRef buffer;                ///< Buffer, for UniformBuffer / StorageBuffer slots.
    std::uint64_t offset = 0;        ///< Byte offset into `buffer`.
    std::uint64_t size = kWholeSize; ///< Bytes of `buffer` visible to the shader.
    TextureViewRef texture_view;     ///< View, for SampledTexture / StorageTexture slots.
    SamplerRef sampler;              ///< Sampler, for Sampler slots.
};

/// Describes a bind group to create with IDevice::createBindGroup.
struct BindGroupDesc {
    BindGroupLayoutRef layout;           ///< Layout the entries must match; retained.
    std::vector<BindGroupEntry> entries; ///< One entry per (slot, array element).
    std::string label;                   ///< Debug name.
};

/// Binds a bind group at `index` of the current pipeline's layout (IRenderEncoder / IComputeEncoder).
struct BindGroupBinding {
    std::uint32_t index = 0; ///< Index into PipelineLayoutDesc::bind_group_layouts (`set = N` in the shader).
    BindGroupRef bind_group; ///< Group to bind; its layout must match the pipeline layout at that index.
};

/// A set of resources matching a bind group layout (a Vulkan descriptor set).
///
/// Retains its layout and every bound resource, so they stay alive while the group does.
class IBindGroup {
public:
    virtual ~IBindGroup() = default;

    /// The descriptor the group was created with.
    virtual BindGroupDesc getDesc() const = 0;
};

} // namespace rhi
