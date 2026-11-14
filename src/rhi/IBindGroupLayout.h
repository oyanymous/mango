#pragma once

/**
 * @file
 * @brief rhi::IBindGroupLayout: the shape of a set of shader resources.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

/**
 * @brief The kind of resource in a binding slot.
 */
enum class BindingType {
    UniformBuffer,  /**< Read-only uniform buffer (BufferUsage::Uniform). */
    StorageBuffer,  /**< Read-write storage buffer (BufferUsage::Storage). */
    SampledTexture, /**< Texture view read through a separate sampler (TextureUsage::Sampled). */
    StorageTexture, /**< Read-write storage image (TextureUsage::Storage). */
    Sampler,        /**< A sampler, combined with a SampledTexture in the shader. */
};

/**
 * @brief One binding slot of a bind group layout.
 */
struct BindGroupLayoutEntry {
    std::uint32_t binding = 0;                     /**< Slot number; matches `binding = N` in the shader. */
    BindingType type = BindingType::UniformBuffer; /**< Kind of resource. */
    ShaderStage visibility = ShaderStage::None;    /**< Stages that access the slot. */
    std::uint32_t count = 1;                       /**< Array size; > 1 for arrays of resources. */
};

/**
 * @brief Describes a bind group layout to create with IDevice::createBindGroupLayout.
 */
struct BindGroupLayoutDesc {
    std::vector<BindGroupLayoutEntry> entries; /**< The slots; binding numbers must be unique. */
    std::string label;                         /**< Debug name. */
};

/**
 * @brief The shape of a bind group (a Vulkan descriptor set layout): which kind of resource goes in which slot.
 *
 * @details
 * Pipeline layouts are built from bind group layouts, and every IBindGroup is created against one.
 */
class IBindGroupLayout {
public:
    virtual ~IBindGroupLayout() = default;

    /**
     * @brief The descriptor the layout was created with.
     */
    virtual BindGroupLayoutDesc getDesc() const = 0;
};

} // namespace rhi
