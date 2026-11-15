#pragma once

#include <memory>

#include "ISampler.h"
#include "ITexture.h"
#include "ITextureView.h"
#include "Result.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"

namespace rhi::vulkan {

class VulkanTexture final : public ITexture {
public:
    static Result<TextureRef> create(std::shared_ptr<VulkanDeviceHandle> device, TextureDesc desc);

    VulkanTexture(std::shared_ptr<VulkanDeviceHandle> device, TextureDesc desc, VmaAllocationPtr allocation,
                  vk::raii::Image image);

    VulkanTexture(std::shared_ptr<VulkanDeviceHandle> device, TextureDesc desc, vk::Image borrowed_image);

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    TextureDesc getDesc() const override;

    vk::Image handle() const { return m_handle; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    TextureDesc m_desc;
    VmaAllocationPtr m_allocation;
    vk::raii::Image m_image;
    vk::Image m_handle;
};

class VulkanTextureView final : public ITextureView {
public:
    static Result<TextureViewRef> create(std::shared_ptr<VulkanDeviceHandle> device, TextureViewDesc desc);

    VulkanTextureView(std::shared_ptr<VulkanDeviceHandle> device, TextureViewDesc desc, vk::raii::ImageView image_view);

    VulkanTextureView(const VulkanTextureView&) = delete;
    VulkanTextureView& operator=(const VulkanTextureView&) = delete;

    TextureRef getTexture() const override;

    vk::ImageView handle() const { return *m_image_view; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    TextureViewDesc m_desc;
    vk::raii::ImageView m_image_view;
};

class VulkanSampler final : public ISampler {
public:
    static Result<SamplerRef> create(std::shared_ptr<VulkanDeviceHandle> device, SamplerDesc desc,
                                     bool anisotropy_enabled);

    VulkanSampler(std::shared_ptr<VulkanDeviceHandle> device, SamplerDesc desc, vk::raii::Sampler sampler);

    VulkanSampler(const VulkanSampler&) = delete;
    VulkanSampler& operator=(const VulkanSampler&) = delete;

    SamplerDesc getDesc() const override;

    vk::Sampler handle() const { return *m_sampler; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    SamplerDesc m_desc;
    vk::raii::Sampler m_sampler;
};

} // namespace rhi::vulkan
