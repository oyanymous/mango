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

    /// Takes ownership of a created VkDescriptorSetLayout. Use create().
    VulkanBindGroupLayout(std::shared_ptr<VulkanDeviceHandle> device, BindGroupLayoutDesc desc,
                          VkDescriptorSetLayout layout);
    ~VulkanBindGroupLayout() override;

    VulkanBindGroupLayout(const VulkanBindGroupLayout&) = delete;
    VulkanBindGroupLayout& operator=(const VulkanBindGroupLayout&) = delete;

    BindGroupLayoutDesc getDesc() const override;

    /// The VkDescriptorSetLayout, for pipeline layouts and descriptor set allocation.
    VkDescriptorSetLayout handle() const { return m_layout; }
private:
    BindGroupLayoutDesc m_desc;
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
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
     * @param pool The pool @p set was allocated from.
     */
    VulkanBindGroup(std::shared_ptr<VulkanDeviceHandle> device, BindGroupDesc desc, VkDescriptorPool pool,
                    VkDescriptorSet set);
    /// Destroys the pool, which frees the set.
    ~VulkanBindGroup() override;

    VulkanBindGroup(const VulkanBindGroup&) = delete;
    VulkanBindGroup& operator=(const VulkanBindGroup&) = delete;

    BindGroupDesc getDesc() const override;

    /// The VkDescriptorSet, for vkCmdBindDescriptorSets.
    VkDescriptorSet handle() const { return m_set; }
private:
    BindGroupDesc m_desc;
    VkDescriptorPool m_pool = VK_NULL_HANDLE;
    VkDescriptorSet m_set = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
