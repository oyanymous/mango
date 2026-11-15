#include "MetalBindGroup.h"

#include <algorithm>
#include <utility>

#include "IBindGroupLayout.h"
#include "MetalBuffer.h"
#include "MetalPipeline.h"
#include "MetalRenderEncoder.h"
#include "MetalTexture.h"

namespace rhi::metal {

namespace {

const MetalBoundResource* find(const std::vector<MetalBoundResource>& resources, std::uint32_t binding,
                               std::uint32_t element) {
    auto it = std::ranges::find_if(resources, [&](const MetalBoundResource& resource) {
        return resource.binding == binding && resource.array_element == element;
    });
    return it == resources.end() ? nullptr : &*it;
}

template <typename Bind>
void forEachBinding(const std::vector<MetalBoundResource>& resources, const MetalPipelineLayout& layout,
                    std::uint32_t set, ShaderStage stage, Bind&& bind) {
    if (set >= layout.sets().size()) {
        return;
    }
    for (const MetalBindingSlot& slot : layout.sets()[set]) {
        if (slot.visibility != ShaderStage::None && !hasFlag(slot.visibility, stage)) {
            continue;
        }
        for (std::uint32_t element = 0; element < slot.count; ++element) {
            if (const MetalBoundResource* resource = find(resources, slot.binding, element)) {
                bind(*resource, static_cast<NSUInteger>(slot.index + element));
            }
        }
    }
}

id<MTLTexture> textureOf(const MetalBoundResource& resource) {
    return static_cast<const MetalTextureView&>(*resource.texture_view).handle();
}

} // namespace

Result<BindGroupRef> MetalBindGroup::create(BindGroupDesc desc) {
    if (!desc.layout) {
        return Error{ErrorCode::InvalidArgument, "createBindGroup: layout is null"};
    }
    const BindGroupLayoutDesc layout = desc.layout->getDesc();
    std::vector<MetalBoundResource> resources;
    for (const BindGroupEntry& entry : desc.entries) {
        auto slot = std::ranges::find(layout.entries, entry.binding, &BindGroupLayoutEntry::binding);
        if (slot == layout.entries.end() || entry.array_element >= slot->count) {
            return Error{ErrorCode::InvalidArgument, "createBindGroup: binding or array element not in the layout"};
        }
        MetalBoundResource resource{.binding = entry.binding, .array_element = entry.array_element, .type = slot->type};
        switch (slot->type) {
        case BindingType::UniformBuffer:
        case BindingType::StorageBuffer:
            if (!entry.buffer || entry.offset >= entry.buffer->getDesc().size) {
                return Error{ErrorCode::InvalidArgument, "createBindGroup: buffer slot without a buffer, or offset "
                                                         "past its end"};
            }
            resource.buffer = static_cast<const MetalBuffer&>(*entry.buffer).handle();
            resource.offset = entry.offset;
            break;
        case BindingType::SampledTexture:
        case BindingType::StorageTexture:
            if (!entry.texture_view) {
                return Error{ErrorCode::InvalidArgument, "createBindGroup: texture slot without a texture view"};
            }
            resource.texture_view = entry.texture_view;
            break;
        case BindingType::Sampler:
            if (!entry.sampler) {
                return Error{ErrorCode::InvalidArgument, "createBindGroup: sampler slot without a sampler"};
            }
            resource.sampler = static_cast<const MetalSampler&>(*entry.sampler).handle();
            break;
        }
        resources.push_back(std::move(resource));
    }
    return BindGroupRef(std::make_shared<MetalBindGroup>(std::move(desc), std::move(resources)));
}

MetalBindGroup::MetalBindGroup(BindGroupDesc desc, std::vector<MetalBoundResource> resources)
    : m_desc(std::move(desc)), m_resources(std::move(resources)) {
}

BindGroupDesc MetalBindGroup::getDesc() const {
    return m_desc;
}

void MetalBindGroup::bind(MetalRenderState& state, const MetalPipelineLayout& layout, std::uint32_t set) const {
    for (ShaderStage stage : {ShaderStage::Vertex, ShaderStage::Fragment}) {
        forEachBinding(m_resources, layout, set, stage, [&](const MetalBoundResource& resource, NSUInteger index) {
            switch (resource.type) {
            case BindingType::UniformBuffer:
            case BindingType::StorageBuffer:
                state.setBuffer(stage, resource.buffer, resource.offset, index);
                break;
            case BindingType::SampledTexture:
            case BindingType::StorageTexture:
                state.setTexture(stage, textureOf(resource), index);
                break;
            case BindingType::Sampler:
                state.setSampler(stage, resource.sampler, index);
                break;
            }
        });
    }
}

void MetalBindGroup::bind(id<MTLComputeCommandEncoder> encoder, const MetalPipelineLayout& layout,
                          std::uint32_t set) const {
    forEachBinding(m_resources, layout, set, ShaderStage::Compute,
                   [&](const MetalBoundResource& resource, NSUInteger index) {
                       switch (resource.type) {
                       case BindingType::UniformBuffer:
                       case BindingType::StorageBuffer:
                           [encoder setBuffer:resource.buffer offset:resource.offset atIndex:index];
                           break;
                       case BindingType::SampledTexture:
                       case BindingType::StorageTexture:
                           [encoder setTexture:textureOf(resource) atIndex:index];
                           break;
                       case BindingType::Sampler:
                           [encoder setSamplerState:resource.sampler atIndex:index];
                           break;
                       }
                   });
}

} // namespace rhi::metal
