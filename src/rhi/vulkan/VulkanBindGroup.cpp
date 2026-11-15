#include "VulkanBindGroup.h"

#include <map>
#include <set>
#include <vector>

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"

namespace rhi::vulkan {

namespace {

vk::DescriptorType toVkDescriptorType(BindingType type) {
    switch (type) {
    case BindingType::UniformBuffer:
        return vk::DescriptorType::eUniformBuffer;
    case BindingType::StorageBuffer:
        return vk::DescriptorType::eStorageBuffer;
    case BindingType::SampledTexture:
        return vk::DescriptorType::eSampledImage;
    case BindingType::StorageTexture:
        return vk::DescriptorType::eStorageImage;
    case BindingType::Sampler:
        return vk::DescriptorType::eSampler;
    }
    return vk::DescriptorType::eUniformBuffer;
}

vk::ShaderStageFlags toVkShaderStages(ShaderStage stages) {
    vk::ShaderStageFlags flags;
    if (hasFlag(stages, ShaderStage::Vertex)) {
        flags |= vk::ShaderStageFlagBits::eVertex;
    }
    if (hasFlag(stages, ShaderStage::Fragment)) {
        flags |= vk::ShaderStageFlagBits::eFragment;
    }
    if (hasFlag(stages, ShaderStage::Compute)) {
        flags |= vk::ShaderStageFlagBits::eCompute;
    }
    return flags;
}

} // namespace

Result<BindGroupLayoutRef> VulkanBindGroupLayout::create(std::shared_ptr<VulkanDeviceHandle> device,
                                                         BindGroupLayoutDesc desc) {
    std::set<std::uint32_t> seen;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    for (const BindGroupLayoutEntry& entry : desc.entries) {
        if (!seen.insert(entry.binding).second) {
            return Error{ErrorCode::InvalidArgument, "createBindGroupLayout: a binding number repeats"};
        }
        if (entry.count == 0 || entry.visibility == ShaderStage::None) {
            return Error{ErrorCode::InvalidArgument,
                         "createBindGroupLayout: every entry needs count > 0 and a visible stage"};
        }
        bindings.push_back({
            .binding = entry.binding,
            .descriptorType = toVkDescriptorType(entry.type),
            .descriptorCount = entry.count,
            .stageFlags = toVkShaderStages(entry.visibility),
        });
    }

    const vk::DescriptorSetLayoutCreateInfo info{
        .bindingCount = static_cast<std::uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };
    Result<vk::raii::DescriptorSetLayout> layout =
        check(device->m_device.createDescriptorSetLayout(info, nullptr), "vkCreateDescriptorSetLayout");
    if (!layout) {
        return layout.error();
    }
    return BindGroupLayoutRef(
        std::make_shared<VulkanBindGroupLayout>(std::move(device), std::move(desc), std::move(layout).value()));
}

VulkanBindGroupLayout::VulkanBindGroupLayout(std::shared_ptr<VulkanDeviceHandle> device, BindGroupLayoutDesc desc,
                                             vk::raii::DescriptorSetLayout layout)
    : m_device(std::move(device)), m_desc(std::move(desc)), m_layout(std::move(layout)) {
}

BindGroupLayoutDesc VulkanBindGroupLayout::getDesc() const {
    return m_desc;
}

Result<BindGroupRef> VulkanBindGroup::create(std::shared_ptr<VulkanDeviceHandle> device, BindGroupDesc desc) {
    if (!desc.layout) {
        return Error{ErrorCode::InvalidArgument, "createBindGroup: layout is null"};
    }
    const BindGroupLayoutDesc layout_desc = desc.layout->getDesc();

    std::map<std::uint32_t, const BindGroupLayoutEntry*> slots;
    for (const BindGroupLayoutEntry& slot : layout_desc.entries) {
        slots[slot.binding] = &slot;
    }
    for (const BindGroupEntry& entry : desc.entries) {
        auto it = slots.find(entry.binding);
        if (it == slots.end() || entry.array_element >= it->second->count) {
            return Error{ErrorCode::InvalidArgument, "createBindGroup: an entry's binding is not in the layout"};
        }
        const BindingType type = it->second->type;
        const bool is_buffer = type == BindingType::UniformBuffer || type == BindingType::StorageBuffer;
        const bool is_texture = type == BindingType::SampledTexture || type == BindingType::StorageTexture;
        if ((is_buffer && !entry.buffer) || (is_texture && !entry.texture_view) ||
            (type == BindingType::Sampler && !entry.sampler)) {
            return Error{ErrorCode::InvalidArgument, "createBindGroup: an entry lacks the resource its slot needs"};
        }
    }

    std::map<vk::DescriptorType, std::uint32_t> counts;
    for (const BindGroupLayoutEntry& slot : layout_desc.entries) {
        counts[toVkDescriptorType(slot.type)] += slot.count;
    }
    std::vector<vk::DescriptorPoolSize> sizes;
    for (const auto& [type, count] : counts) {
        sizes.push_back({.type = type, .descriptorCount = count});
    }
    const vk::DescriptorPoolCreateInfo pool_info{
        .maxSets = 1,
        .poolSizeCount = static_cast<std::uint32_t>(sizes.size()),
        .pPoolSizes = sizes.data(),
    };
    Result<vk::raii::DescriptorPool> pool =
        check(device->m_device.createDescriptorPool(pool_info, nullptr), "vkCreateDescriptorPool");
    if (!pool) {
        return pool.error();
    }

    const vk::DescriptorSetLayout set_layout = static_cast<const VulkanBindGroupLayout&>(*desc.layout).handle();
    const vk::DescriptorSetAllocateInfo alloc_info{
        .descriptorPool = *pool.value(),
        .descriptorSetCount = 1,
        .pSetLayouts = &set_layout,
    };
    Result<std::vector<vk::raii::DescriptorSet>> sets =
        check(device->m_device.allocateDescriptorSets(alloc_info), "vkAllocateDescriptorSets");
    if (!sets) {
        return sets.error();
    }

    const vk::DescriptorSet set = sets.value().front().release();

    std::vector<vk::DescriptorBufferInfo> buffer_infos;
    std::vector<vk::DescriptorImageInfo> image_infos;
    buffer_infos.reserve(desc.entries.size());
    image_infos.reserve(desc.entries.size());
    std::vector<vk::WriteDescriptorSet> writes;
    for (const BindGroupEntry& entry : desc.entries) {
        const BindingType type = slots[entry.binding]->type;
        vk::WriteDescriptorSet write{
            .dstSet = set,
            .dstBinding = entry.binding,
            .dstArrayElement = entry.array_element,
            .descriptorCount = 1,
            .descriptorType = toVkDescriptorType(type),
        };
        switch (type) {
        case BindingType::UniformBuffer:
        case BindingType::StorageBuffer:

            buffer_infos.push_back({
                .buffer = static_cast<const VulkanBuffer&>(*entry.buffer).handle(),
                .offset = entry.offset,
                .range = entry.size,
            });
            write.pBufferInfo = &buffer_infos.back();
            break;
        case BindingType::SampledTexture:
        case BindingType::StorageTexture:

            image_infos.push_back({
                .imageView = static_cast<const VulkanTextureView&>(*entry.texture_view).handle(),
                .imageLayout = type == BindingType::SampledTexture ? vk::ImageLayout::eShaderReadOnlyOptimal
                                                                   : vk::ImageLayout::eGeneral,
            });
            write.pImageInfo = &image_infos.back();
            break;
        case BindingType::Sampler:
            image_infos.push_back({.sampler = static_cast<const VulkanSampler&>(*entry.sampler).handle()});
            write.pImageInfo = &image_infos.back();
            break;
        }
        writes.push_back(write);
    }
    device->m_device.updateDescriptorSets(writes, nullptr);

    return BindGroupRef(
        std::make_shared<VulkanBindGroup>(std::move(device), std::move(desc), std::move(pool).value(), set));
}

VulkanBindGroup::VulkanBindGroup(std::shared_ptr<VulkanDeviceHandle> device, BindGroupDesc desc,
                                 vk::raii::DescriptorPool pool, vk::DescriptorSet set)
    : m_device(std::move(device)), m_desc(std::move(desc)), m_pool(std::move(pool)), m_set(set) {
}

BindGroupDesc VulkanBindGroup::getDesc() const {
    return m_desc;
}

} // namespace rhi::vulkan
