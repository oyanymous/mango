/**
 * @file
 * @brief VulkanBindGroupLayout and VulkanBindGroup.
 */

#include "VulkanBindGroup.h"

#include <map>
#include <set>
#include <vector>

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"

namespace rhi::vulkan {

namespace {

VkDescriptorType toVkDescriptorType(BindingType type) {
    switch (type) {
    case BindingType::UniformBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case BindingType::StorageBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case BindingType::SampledTexture:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case BindingType::StorageTexture:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case BindingType::Sampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    }
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
}

VkShaderStageFlags toVkShaderStages(ShaderStage stages) {
    VkShaderStageFlags flags = 0;
    if (hasFlag(stages, ShaderStage::Vertex)) {
        flags |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (hasFlag(stages, ShaderStage::Fragment)) {
        flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if (hasFlag(stages, ShaderStage::Compute)) {
        flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    }
    return flags;
}

} // namespace

// --- VulkanBindGroupLayout ---

Result<BindGroupLayoutRef> VulkanBindGroupLayout::create(std::shared_ptr<VulkanDeviceHandle> device,
                                                         BindGroupLayoutDesc desc) {
    std::set<std::uint32_t> seen;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (const BindGroupLayoutEntry& entry : desc.entries) {
        if (!seen.insert(entry.binding).second) {
            return Error{ErrorCode::InvalidArgument, "createBindGroupLayout: a binding number repeats"};
        }
        if (entry.count == 0 || entry.visibility == ShaderStage::None) {
            return Error{ErrorCode::InvalidArgument,
                         "createBindGroupLayout: every entry needs count > 0 and a visible stage"};
        }
        bindings.push_back(
            {entry.binding, toVkDescriptorType(entry.type), entry.count, toVkShaderStages(entry.visibility), nullptr});
    }

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<std::uint32_t>(bindings.size());
    info.pBindings = bindings.data();
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    if (VkResult result = vkCreateDescriptorSetLayout(device->vkDevice(), &info, nullptr, &layout);
        result != VK_SUCCESS) {
        return toError(result, "vkCreateDescriptorSetLayout");
    }
    return BindGroupLayoutRef(std::make_shared<VulkanBindGroupLayout>(std::move(device), std::move(desc), layout));
}

VulkanBindGroupLayout::VulkanBindGroupLayout(std::shared_ptr<VulkanDeviceHandle> device, BindGroupLayoutDesc desc,
                                             VkDescriptorSetLayout layout)
    : m_desc(std::move(desc)), m_layout(layout), m_device(std::move(device)) {
}

VulkanBindGroupLayout::~VulkanBindGroupLayout() {
    vkDestroyDescriptorSetLayout(m_device->vkDevice(), m_layout, nullptr);
}

BindGroupLayoutDesc VulkanBindGroupLayout::getDesc() const {
    return m_desc;
}

// --- VulkanBindGroup ---

Result<BindGroupRef> VulkanBindGroup::create(std::shared_ptr<VulkanDeviceHandle> device, BindGroupDesc desc) {
    if (!desc.layout) {
        return Error{ErrorCode::InvalidArgument, "createBindGroup: layout is null"};
    }
    const BindGroupLayoutDesc layout_desc = desc.layout->getDesc();

    // Resolve every entry against the layout before touching Vulkan.
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

    // shortcut: one pool per group, sized for its layout; switch to shared pools if groups get created per frame.
    std::map<VkDescriptorType, std::uint32_t> counts;
    for (const BindGroupLayoutEntry& slot : layout_desc.entries) {
        counts[toVkDescriptorType(slot.type)] += slot.count;
    }
    std::vector<VkDescriptorPoolSize> sizes;
    for (const auto& [type, count] : counts) {
        sizes.push_back({type, count});
    }
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = static_cast<std::uint32_t>(sizes.size());
    pool_info.pPoolSizes = sizes.data();
    VkDescriptorPool pool = VK_NULL_HANDLE;
    if (VkResult result = vkCreateDescriptorPool(device->vkDevice(), &pool_info, nullptr, &pool);
        result != VK_SUCCESS) {
        return toError(result, "vkCreateDescriptorPool");
    }

    const VkDescriptorSetLayout set_layout = static_cast<const VulkanBindGroupLayout&>(*desc.layout).handle();
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &set_layout;
    VkDescriptorSet set = VK_NULL_HANDLE;
    if (VkResult result = vkAllocateDescriptorSets(device->vkDevice(), &alloc_info, &set); result != VK_SUCCESS) {
        vkDestroyDescriptorPool(device->vkDevice(), pool, nullptr);
        return toError(result, "vkAllocateDescriptorSets");
    }

    // Reserved up front: the writes point into these vectors.
    std::vector<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkDescriptorImageInfo> image_infos;
    buffer_infos.reserve(desc.entries.size());
    image_infos.reserve(desc.entries.size());
    std::vector<VkWriteDescriptorSet> writes;
    for (const BindGroupEntry& entry : desc.entries) {
        const BindingType type = slots[entry.binding]->type;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.dstBinding = entry.binding;
        write.dstArrayElement = entry.array_element;
        write.descriptorCount = 1;
        write.descriptorType = toVkDescriptorType(type);
        switch (type) {
        case BindingType::UniformBuffer:
        case BindingType::StorageBuffer:
            // kWholeSize is VK_WHOLE_SIZE.
            buffer_infos.push_back(
                {static_cast<const VulkanBuffer&>(*entry.buffer).handle(), entry.offset, entry.size});
            write.pBufferInfo = &buffer_infos.back();
            break;
        case BindingType::SampledTexture:
        case BindingType::StorageTexture:
            // The layouts ResourceState::ShaderRead and ShaderWrite transition to.
            image_infos.push_back({VK_NULL_HANDLE, static_cast<const VulkanTextureView&>(*entry.texture_view).handle(),
                                   type == BindingType::SampledTexture ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                                       : VK_IMAGE_LAYOUT_GENERAL});
            write.pImageInfo = &image_infos.back();
            break;
        case BindingType::Sampler:
            image_infos.push_back({static_cast<const VulkanSampler&>(*entry.sampler).handle(), VK_NULL_HANDLE,
                                   VK_IMAGE_LAYOUT_UNDEFINED});
            write.pImageInfo = &image_infos.back();
            break;
        }
        writes.push_back(write);
    }
    vkUpdateDescriptorSets(device->vkDevice(), static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);

    return BindGroupRef(std::make_shared<VulkanBindGroup>(std::move(device), std::move(desc), pool, set));
}

VulkanBindGroup::VulkanBindGroup(std::shared_ptr<VulkanDeviceHandle> device, BindGroupDesc desc, VkDescriptorPool pool,
                                 VkDescriptorSet set)
    : m_desc(std::move(desc)), m_pool(pool), m_set(set), m_device(std::move(device)) {
}

VulkanBindGroup::~VulkanBindGroup() {
    vkDestroyDescriptorPool(m_device->vkDevice(), m_pool, nullptr);
}

BindGroupDesc VulkanBindGroup::getDesc() const {
    return m_desc;
}

} // namespace rhi::vulkan
