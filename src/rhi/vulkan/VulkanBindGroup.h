#pragma once

#include <memory>

#include "IBindGroup.h"
#include "IBindGroupLayout.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/// The Vulkan IBindGroupLayout: a VkDescriptorSetLayout. Created by VulkanDevice::createBindGroupLayout.
class VulkanBindGroupLayout final : public IBindGroupLayout {
public:
    /**
     * Creates the VkDescriptorSetLayout.
     *
     * @return The layout; InvalidArgument when a binding number repeats, a count is 0 or an entry has no
     *         visible stage.
     */
    static Result<BindGroupLayoutRef> create(std::shared_ptr<VulkanDeviceHandle> device, BindGroupLayoutDesc desc);

    /// Takes ownership of a created descriptor set layout. Use create().
    VulkanBindGroupLayout(std::shared_ptr<VulkanDeviceHandle> device, BindGroupLayoutDesc desc,
                          vk::raii::DescriptorSetLayout layout);

    VulkanBindGroupLayout(const VulkanBindGroupLayout&) = delete;
    VulkanBindGroupLayout& operator=(const VulkanBindGroupLayout&) = delete;

    BindGroupLayoutDesc getDesc() const override;

    vk::DescriptorSetLayout handle() const { return *m_layout; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device; // first: destroyed after the layout
    BindGroupLayoutDesc m_desc;
    vk::raii::DescriptorSetLayout m_layout;
};

/**
 * The Vulkan IBindGroup: one VkDescriptorSet in a VkDescriptorPool of its own.
 *
 * Retains its layout and every buffer, texture view and sampler it references. The descriptors are written
 * once at creation and never updated.
 */
class VulkanBindGroup final : public IBindGroup {
public:
    /**
     * Allocates the descriptor set and writes every entry into it.
     *
     * @return The group; InvalidArgument when the layout is missing, an entry names a binding or array element
     *         the layout does not have, or lacks the resource its slot type needs.
     */
    static Result<BindGroupRef> create(std::shared_ptr<VulkanDeviceHandle> device, BindGroupDesc desc);

    /**
     * Takes ownership of a created pool and its set. Use create().
     *
     * @param desc The descriptor it was created with (retains the resources).
     * @param pool The pool @p set was allocated from; destroying it frees the set.
     */
    VulkanBindGroup(std::shared_ptr<VulkanDeviceHandle> device, BindGroupDesc desc, vk::raii::DescriptorPool pool,
                    vk::DescriptorSet set);

    VulkanBindGroup(const VulkanBindGroup&) = delete;
    VulkanBindGroup& operator=(const VulkanBindGroup&) = delete;

    BindGroupDesc getDesc() const override;

    vk::DescriptorSet handle() const { return m_set; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device; // first: destroyed after the pool
    BindGroupDesc m_desc;
    // The pool is not created with FREE_DESCRIPTOR_SET, so the set is a plain handle that the pool frees.
    vk::raii::DescriptorPool m_pool;
    vk::DescriptorSet m_set;
};

} // namespace rhi::vulkan
