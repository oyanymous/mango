#pragma once

/**
 * @file
 * @brief rhi::metal::MetalBindGroup: resources resolved for Metal's argument tables.
 */

#include <cstdint>
#include <memory>
#include <vector>

#include "IBindGroup.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalPipelineLayout;

/**
 * @brief One resource of a bind group, ready to bind.
 */
struct MetalBoundResource {
    std::uint32_t binding = 0;                     /**< Layout slot. */
    std::uint32_t array_element = 0;               /**< Element of an array slot. */
    BindingType type = BindingType::UniformBuffer; /**< Kind of resource. */
    id<MTLBuffer> buffer = nil;                    /**< Buffers. */
    std::uint64_t offset = 0;                      /**< Buffers: byte offset. */
    id<MTLSamplerState> sampler = nil;             /**< Samplers. */
    /**
     * @brief Textures: the view. Its MTLTexture is read when the group is bound, so a backbuffer view follows the
     * current drawable.
     */
    TextureViewRef texture_view;
};

/**
 * @brief The resources of one bind group. Metal has no descriptor sets; setBindGroup() calls the encoder's
 * `set*Buffer`/`set*Texture`/`set*SamplerState` for every resource, at the indices the current pipeline's layout
 * assigned to the group's set, in every stage the layout entry is visible to (all stages if its visibility is None).
 */
class MetalBindGroup final : public IBindGroup {
public:
    /**
     * @brief Validates and resolves the entries.
     *
     * @param desc Layout and entries; the resources are retained.
     * @return The group, or InvalidArgument for a null layout, an entry whose binding is not in the layout, an array
     *         element out of range, or a missing resource of the slot's type.
     */
    static Result<BindGroupRef> create(BindGroupDesc desc);

    /**
     * @brief Wraps resolved entries. Use create().
     *
     * @param desc The descriptor (retains the resources).
     * @param resources The resolved resources.
     */
    MetalBindGroup(BindGroupDesc desc, std::vector<MetalBoundResource> resources);

    BindGroupDesc getDesc() const override;

    /**
     * @brief Binds the group as set @p set of @p layout on a render encoder.
     */
    void bind(id<MTLRenderCommandEncoder> encoder, const MetalPipelineLayout& layout, std::uint32_t set) const;

    /**
     * @brief Binds the group as set @p set of @p layout on a compute encoder.
     */
    void bind(id<MTLComputeCommandEncoder> encoder, const MetalPipelineLayout& layout, std::uint32_t set) const;
private:
    BindGroupDesc m_desc;
    std::vector<MetalBoundResource> m_resources;
};

} // namespace rhi::metal
