#pragma once

/**
 * @file
 * @brief rhi::vulkan::VulkanTexture, VulkanTextureView and VulkanSampler.
 */

#include <memory>

#include <vk_mem_alloc.h>

#include "ISampler.h"
#include "ITexture.h"
#include "ITextureView.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/**
 * @brief The Vulkan ITexture: a VkImage with VMA-allocated memory. Created by VulkanDevice::createTexture.
 *
 * @details
 * Layouts are not tracked per texture: barriers name both states, and each state maps to one layout.
 */
class VulkanTexture final : public ITexture {
public:
    /**
     * @brief Creates the image and its memory.
     *
     * @param device Owner of the VkDevice and allocator; kept alive by the texture.
     * @param desc Shape, format and usage.
     * @return The texture, or InvalidArgument for an Undefined format, no usage, a zero extent, or a cube
     *         whose layer count is not a multiple of 6.
     */
    static Result<TextureRef> create(std::shared_ptr<VulkanDeviceHandle> device, TextureDesc desc);

    /**
     * @brief Takes ownership of a created image and allocation. Use create().
     *
     * @param device Owner of the VkDevice.
     * @param desc The descriptor it was created with.
     * @param image The image.
     * @param allocation Its memory, or VK_NULL_HANDLE for an image owned elsewhere (a swapchain backbuffer),
     *                   which is then not destroyed with the texture.
     */
    VulkanTexture(std::shared_ptr<VulkanDeviceHandle> device, TextureDesc desc, VkImage image,
                  VmaAllocation allocation);
    /**
     * @brief Destroys the image and frees its memory, unless the image is owned elsewhere.
     */
    ~VulkanTexture() override;

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    TextureDesc getDesc() const override;

    /**
     * @brief The VkImage, for recording commands and creating views.
     */
    VkImage handle() const { return m_image; }
private:
    TextureDesc m_desc;
    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

/**
 * @brief The Vulkan ITextureView: a VkImageView. Created by VulkanDevice::createTextureView.
 */
class VulkanTextureView final : public ITextureView {
public:
    /**
     * @brief Creates the image view.
     *
     * @param device Owner of the VkDevice; kept alive by the view.
     * @param desc Texture, format, dimension, aspect and range.
     * @return The view, or InvalidArgument for a null texture.
     */
    static Result<TextureViewRef> create(std::shared_ptr<VulkanDeviceHandle> device, TextureViewDesc desc);

    /**
     * @brief Takes ownership of a created image view. Use create().
     *
     * @param device Owner of the VkDevice.
     * @param desc The descriptor it was created with (retains the texture).
     * @param image_view The view.
     */
    VulkanTextureView(std::shared_ptr<VulkanDeviceHandle> device, TextureViewDesc desc, VkImageView image_view);
    /**
     * @brief Destroys the image view.
     */
    ~VulkanTextureView() override;

    VulkanTextureView(const VulkanTextureView&) = delete;
    VulkanTextureView& operator=(const VulkanTextureView&) = delete;

    TextureRef getTexture() const override;

    /**
     * @brief The VkImageView, for framebuffers and descriptors.
     */
    VkImageView handle() const { return m_image_view; }
private:
    TextureViewDesc m_desc;
    VkImageView m_image_view = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

/**
 * @brief The Vulkan ISampler: a VkSampler. Created by VulkanDevice::createSampler.
 */
class VulkanSampler final : public ISampler {
public:
    /**
     * @brief Creates the sampler.
     *
     * @param device Owner of the VkDevice; kept alive by the sampler.
     * @param desc Filtering, addressing and comparison.
     * @param anisotropy_enabled Whether the device was created with DeviceFeatures::sampler_anisotropy.
     * @return The sampler, or Unsupported if `max_anisotropy > 1` without that feature.
     */
    static Result<SamplerRef> create(std::shared_ptr<VulkanDeviceHandle> device, SamplerDesc desc,
                                     bool anisotropy_enabled);

    /**
     * @brief Takes ownership of a created sampler. Use create().
     *
     * @param device Owner of the VkDevice.
     * @param desc The descriptor it was created with.
     * @param sampler The sampler.
     */
    VulkanSampler(std::shared_ptr<VulkanDeviceHandle> device, SamplerDesc desc, VkSampler sampler);
    /**
     * @brief Destroys the sampler.
     */
    ~VulkanSampler() override;

    VulkanSampler(const VulkanSampler&) = delete;
    VulkanSampler& operator=(const VulkanSampler&) = delete;

    SamplerDesc getDesc() const override;

    /**
     * @brief The VkSampler, for descriptors.
     */
    VkSampler handle() const { return m_sampler; }
private:
    SamplerDesc m_desc;
    VkSampler m_sampler = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
