#pragma once

#include <memory>

#include "IBindGroup.h"
#include "IBindGroupLayout.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

class VulkanBindGroupLayout final : public IBindGroupLayout {
public:
    static Result<BindGroupLayoutRef> create(std::shared_ptr<VulkanDeviceHandle> device, BindGroupLayoutDesc desc);

    VulkanBindGroupLayout(std::shared_ptr<VulkanDeviceHandle> device, BindGroupLayoutDesc desc,
                          vk::raii::DescriptorSetLayout layout);

    VulkanBindGroupLayout(const VulkanBindGroupLayout&) = delete;
    VulkanBindGroupLayout& operator=(const VulkanBindGroupLayout&) = delete;

    BindGroupLayoutDesc getDesc() const override;

    vk::DescriptorSetLayout handle() const { return *m_layout; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    BindGroupLayoutDesc m_desc;
    vk::raii::DescriptorSetLayout m_layout;
};

class VulkanBindGroup final : public IBindGroup {
public:
    static Result<BindGroupRef> create(std::shared_ptr<VulkanDeviceHandle> device, BindGroupDesc desc);

    VulkanBindGroup(std::shared_ptr<VulkanDeviceHandle> device, BindGroupDesc desc, vk::raii::DescriptorPool pool,
                    vk::DescriptorSet set);

    VulkanBindGroup(const VulkanBindGroup&) = delete;
    VulkanBindGroup& operator=(const VulkanBindGroup&) = delete;

    BindGroupDesc getDesc() const override;

    vk::DescriptorSet handle() const { return m_set; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    BindGroupDesc m_desc;

    vk::raii::DescriptorPool m_pool;
    vk::DescriptorSet m_set;
};

} // namespace rhi::vulkan
