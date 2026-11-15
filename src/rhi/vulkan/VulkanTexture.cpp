/**
 * @file
 * @brief VulkanTexture, VulkanTextureView and VulkanSampler.
 */

#include "VulkanTexture.h"

#include "VulkanDevice.h"

namespace rhi::vulkan {

namespace {

/// Maps TextureUsage bits to VkImageUsageFlags.
VkImageUsageFlags toVkImageUsage(TextureUsage usage) {
    VkImageUsageFlags flags = 0;
    if (hasFlag(usage, TextureUsage::CopySrc)) {
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (hasFlag(usage, TextureUsage::CopyDst)) {
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (hasFlag(usage, TextureUsage::Sampled)) {
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (hasFlag(usage, TextureUsage::Storage)) {
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (hasFlag(usage, TextureUsage::RenderTarget)) {
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (hasFlag(usage, TextureUsage::DepthStencil)) {
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    return flags;
}

/// The view type for a view of @p texture with an optional explicit dimension.
vk::ImageViewType toVkViewType(const TextureDesc& texture, std::optional<TextureViewDimension> dimension,
                               std::uint32_t layer_count) {
    if (!dimension) {
        switch (texture.dimension) {
        case TextureDimension::Texture3D:
            return vk::ImageViewType::e3D;
        case TextureDimension::TextureCube:
            return vk::ImageViewType::eCube;
        case TextureDimension::Texture2D:
            return layer_count == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray;
        }
    }
    switch (*dimension) {
    case TextureViewDimension::Texture2D:
        return vk::ImageViewType::e2D;
    case TextureViewDimension::Texture2DArray:
        return vk::ImageViewType::e2DArray;
    case TextureViewDimension::Texture3D:
        return vk::ImageViewType::e3D;
    case TextureViewDimension::TextureCube:
        return vk::ImageViewType::eCube;
    }
    return vk::ImageViewType::e2D;
}

vk::Filter toVkFilter(Filter filter) {
    return filter == Filter::Linear ? vk::Filter::eLinear : vk::Filter::eNearest;
}

vk::SamplerAddressMode toVkAddressMode(AddressMode mode) {
    switch (mode) {
    case AddressMode::Repeat:
        return vk::SamplerAddressMode::eRepeat;
    case AddressMode::MirroredRepeat:
        return vk::SamplerAddressMode::eMirroredRepeat;
    case AddressMode::ClampToEdge:
        return vk::SamplerAddressMode::eClampToEdge;
    }
    return vk::SamplerAddressMode::eRepeat;
}

} // namespace

Result<TextureRef> VulkanTexture::create(std::shared_ptr<VulkanDeviceHandle> device, TextureDesc desc) {
    if (desc.format == Format::Undefined || desc.usage == TextureUsage::None || desc.extent.width == 0 ||
        desc.extent.height == 0 || desc.extent.depth == 0 || desc.mip_level_count == 0 || desc.array_layer_count == 0) {
        return Error{ErrorCode::InvalidArgument, "createTexture: format, usage, extent and counts must be set"};
    }
    const bool cube = desc.dimension == TextureDimension::TextureCube;
    if (cube && desc.array_layer_count % 6 != 0) {
        return Error{ErrorCode::InvalidArgument, "createTexture: a cube needs a multiple of 6 layers"};
    }

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.flags = cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
    image_info.imageType = desc.dimension == TextureDimension::Texture3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    image_info.format = toVkFormat(desc.format);
    image_info.extent = {desc.extent.width, desc.extent.height, desc.extent.depth};
    image_info.mipLevels = desc.mip_level_count;
    image_info.arrayLayers = desc.array_layer_count;
    image_info.samples = static_cast<VkSampleCountFlagBits>(desc.sample_count);
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = toVkImageUsage(desc.usage);
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    if (VkResult result =
            vmaCreateImage(device->m_allocator.get(), &image_info, &allocation_info, &image, &allocation, nullptr);
        result != VK_SUCCESS) {
        return toError(result, "vmaCreateImage");
    }
    // Owned from here on, so an early return cannot leak them.
    VmaAllocationPtr owned_allocation(allocation, {device->m_allocator.get()});
    vk::raii::Image owned_image(device->m_device, image, nullptr);
    return TextureRef(std::make_shared<VulkanTexture>(std::move(device), std::move(desc), std::move(owned_allocation),
                                                      std::move(owned_image)));
}

VulkanTexture::VulkanTexture(std::shared_ptr<VulkanDeviceHandle> device, TextureDesc desc, VmaAllocationPtr allocation,
                             vk::raii::Image image)
    : m_device(std::move(device)),
      m_desc(std::move(desc)),
      m_allocation(std::move(allocation)),
      m_image(std::move(image)),
      m_handle(static_cast<VkImage>(*m_image)) {
}

VulkanTexture::VulkanTexture(std::shared_ptr<VulkanDeviceHandle> device, TextureDesc desc, VkImage borrowed_image)
    : m_device(std::move(device)), m_desc(std::move(desc)), m_image(nullptr), m_handle(borrowed_image) {
}

TextureDesc VulkanTexture::getDesc() const {
    return m_desc;
}

Result<TextureViewRef> VulkanTextureView::create(std::shared_ptr<VulkanDeviceHandle> device, TextureViewDesc desc) {
    if (!desc.texture) {
        return Error{ErrorCode::InvalidArgument, "createTextureView: texture is null"};
    }
    const auto& texture = static_cast<const VulkanTexture&>(*desc.texture);
    const TextureDesc& texture_desc = texture.getDesc();

    vk::ImageAspectFlags aspect(aspectsOf(texture_desc.format));
    if (desc.aspect == TextureAspect::DepthOnly) {
        aspect = vk::ImageAspectFlagBits::eDepth;
    } else if (desc.aspect == TextureAspect::StencilOnly) {
        aspect = vk::ImageAspectFlagBits::eStencil;
    }

    const std::uint32_t layer_count = desc.range.array_layer_count == kRemaining
                                          ? texture_desc.array_layer_count - desc.range.base_array_layer
                                          : desc.range.array_layer_count;

    const vk::ImageViewCreateInfo view_info{
        .image = texture.handle(),
        .viewType = toVkViewType(texture_desc, desc.dimension, layer_count),
        .format = static_cast<vk::Format>(toVkFormat(desc.format.value_or(texture_desc.format))),
        // kRemaining equals VK_REMAINING_MIP_LEVELS / VK_REMAINING_ARRAY_LAYERS (~0u), so the range maps directly.
        .subresourceRange =
            {
                .aspectMask = aspect,
                .baseMipLevel = desc.range.base_mip_level,
                .levelCount = desc.range.mip_level_count,
                .baseArrayLayer = desc.range.base_array_layer,
                .layerCount = desc.range.array_layer_count,
            },
    };

    Result<vk::raii::ImageView> image_view =
        check(device->m_device.createImageView(view_info, nullptr), "vkCreateImageView");
    if (!image_view) {
        return image_view.error();
    }
    return TextureViewRef(
        std::make_shared<VulkanTextureView>(std::move(device), std::move(desc), std::move(image_view).value()));
}

VulkanTextureView::VulkanTextureView(std::shared_ptr<VulkanDeviceHandle> device, TextureViewDesc desc,
                                     vk::raii::ImageView image_view)
    : m_device(std::move(device)), m_desc(std::move(desc)), m_image_view(std::move(image_view)) {
}

TextureRef VulkanTextureView::getTexture() const {
    return m_desc.texture;
}

Result<SamplerRef> VulkanSampler::create(std::shared_ptr<VulkanDeviceHandle> device, SamplerDesc desc,
                                         bool anisotropy_enabled) {
    const bool anisotropy = desc.max_anisotropy > 1.0f;
    if (anisotropy && !anisotropy_enabled) {
        return Error{ErrorCode::Unsupported,
                     "createSampler: max_anisotropy > 1 needs DeviceFeatures::sampler_anisotropy"};
    }

    const vk::SamplerCreateInfo sampler_info{
        .magFilter = toVkFilter(desc.mag_filter),
        .minFilter = toVkFilter(desc.min_filter),
        .mipmapMode =
            desc.mipmap_filter == Filter::Linear ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest,
        .addressModeU = toVkAddressMode(desc.address_u),
        .addressModeV = toVkAddressMode(desc.address_v),
        .addressModeW = toVkAddressMode(desc.address_w),
        .anisotropyEnable = anisotropy ? vk::True : vk::False,
        .maxAnisotropy = anisotropy ? desc.max_anisotropy : 1.0f,
        .compareEnable = desc.compare ? vk::True : vk::False,
        .compareOp = desc.compare ? static_cast<vk::CompareOp>(toVkCompareOp(*desc.compare)) : vk::CompareOp::eNever,
        .minLod = desc.min_lod,
        .maxLod = desc.max_lod,
    };

    Result<vk::raii::Sampler> sampler = check(device->m_device.createSampler(sampler_info, nullptr), "vkCreateSampler");
    if (!sampler) {
        return sampler.error();
    }
    return SamplerRef(std::make_shared<VulkanSampler>(std::move(device), std::move(desc), std::move(sampler).value()));
}

VulkanSampler::VulkanSampler(std::shared_ptr<VulkanDeviceHandle> device, SamplerDesc desc, vk::raii::Sampler sampler)
    : m_device(std::move(device)), m_desc(std::move(desc)), m_sampler(std::move(sampler)) {
}

SamplerDesc VulkanSampler::getDesc() const {
    return m_desc;
}

} // namespace rhi::vulkan
