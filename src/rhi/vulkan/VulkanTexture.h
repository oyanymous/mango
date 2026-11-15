#pragma once

#include <memory>

#include "ISampler.h"
#include "ITexture.h"
#include "ITextureView.h"
#include "Result.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"

namespace rhi::vulkan {

/**
 * The Vulkan ITexture: a VkImage with VMA-allocated memory. Created by VulkanDevice::createTexture.
 *
 * Layouts are not tracked per texture: barriers name both states, and each state maps to one layout.
 */
class VulkanTexture final : public ITexture {
public:
    /**
     * Creates the image and its memory.
     *
     * @return The texture, or InvalidArgument for an Undefined format, no usage, a zero extent, or a cube
     *         whose layer count is not a multiple of 6.
     */
    static Result<TextureRef> create(std::shared_ptr<VulkanDeviceHandle> device, TextureDesc desc);

    /// Takes ownership of a created image and its memory. Use create().
    VulkanTexture(std::shared_ptr<VulkanDeviceHandle> device, TextureDesc desc, VmaAllocationPtr allocation,
                  vk::raii::Image image);
    /// Wraps an image owned elsewhere (a swapchain backbuffer), which is not destroyed with the texture.
    VulkanTexture(std::shared_ptr<VulkanDeviceHandle> device, TextureDesc desc, vk::Image borrowed_image);

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    TextureDesc getDesc() const override;

    vk::Image handle() const { return m_handle; }
private:
    // Destroyed in reverse order: image, then its memory, then (maybe) the device.
    std::shared_ptr<VulkanDeviceHandle> m_device;
    TextureDesc m_desc;
    VmaAllocationPtr m_allocation; // null for a borrowed image
    vk::raii::Image m_image;       // null for a borrowed image
    vk::Image m_handle;
};

/// The Vulkan ITextureView: a VkImageView. Created by VulkanDevice::createTextureView.
class VulkanTextureView final : public ITextureView {
public:
    /**
     * Creates the image view.
     *
     * @return The view, or InvalidArgument for a null texture.
     */
    static Result<TextureViewRef> create(std::shared_ptr<VulkanDeviceHandle> device, TextureViewDesc desc);

    /**
     * Takes ownership of a created image view. Use create().
     *
     * @param desc The descriptor it was created with (retains the texture).
     */
    VulkanTextureView(std::shared_ptr<VulkanDeviceHandle> device, TextureViewDesc desc, vk::raii::ImageView image_view);

    VulkanTextureView(const VulkanTextureView&) = delete;
    VulkanTextureView& operator=(const VulkanTextureView&) = delete;

    TextureRef getTexture() const override;

    vk::ImageView handle() const { return *m_image_view; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device; // first: destroyed after the view
    TextureViewDesc m_desc;
    vk::raii::ImageView m_image_view;
};

/// The Vulkan ISampler: a VkSampler. Created by VulkanDevice::createSampler.
class VulkanSampler final : public ISampler {
public:
    /**
     * Creates the sampler.
     *
     * @param anisotropy_enabled Whether the device was created with DeviceFeatures::sampler_anisotropy.
     * @return The sampler, or Unsupported if `max_anisotropy > 1` without that feature.
     */
    static Result<SamplerRef> create(std::shared_ptr<VulkanDeviceHandle> device, SamplerDesc desc,
                                     bool anisotropy_enabled);

    /// Takes ownership of a created sampler. Use create().
    VulkanSampler(std::shared_ptr<VulkanDeviceHandle> device, SamplerDesc desc, vk::raii::Sampler sampler);

    VulkanSampler(const VulkanSampler&) = delete;
    VulkanSampler& operator=(const VulkanSampler&) = delete;

    SamplerDesc getDesc() const override;

    vk::Sampler handle() const { return *m_sampler; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device; // first: destroyed after the sampler
    SamplerDesc m_desc;
    vk::raii::Sampler m_sampler;
};

} // namespace rhi::vulkan
