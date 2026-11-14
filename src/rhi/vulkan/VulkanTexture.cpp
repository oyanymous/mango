/**
 * @file
 * @brief VulkanTexture, VulkanTextureView and VulkanSampler.
 */

#include "VulkanTexture.h"

#include "VulkanDevice.h"

namespace rhi::vulkan {

namespace {

/**
 * @brief Maps TextureUsage bits to VkImageUsageFlags.
 */
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

/**
 * @brief The view type for a view of @p texture with an optional explicit dimension.
 */
VkImageViewType toVkViewType(const TextureDesc& texture, std::optional<TextureViewDimension> dimension,
                             std::uint32_t layer_count) {
    if (!dimension) {
        switch (texture.dimension) {
        case TextureDimension::Texture3D:
            return VK_IMAGE_VIEW_TYPE_3D;
        case TextureDimension::TextureCube:
            return VK_IMAGE_VIEW_TYPE_CUBE;
        case TextureDimension::Texture2D:
            return layer_count == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        }
    }
    switch (*dimension) {
    case TextureViewDimension::Texture2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case TextureViewDimension::Texture2DArray:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case TextureViewDimension::Texture3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    case TextureViewDimension::TextureCube:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    }
    return VK_IMAGE_VIEW_TYPE_2D;
}

VkFilter toVkFilter(Filter filter) {
    return filter == Filter::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
}

VkSamplerAddressMode toVkAddressMode(AddressMode mode) {
    switch (mode) {
    case AddressMode::Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case AddressMode::MirroredRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case AddressMode::ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
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
            vmaCreateImage(device->m_allocator, &image_info, &allocation_info, &image, &allocation, nullptr);
        result != VK_SUCCESS) {
        return toError(result, "vmaCreateImage");
    }
    return TextureRef(std::make_shared<VulkanTexture>(std::move(device), std::move(desc), image, allocation));
}

VulkanTexture::VulkanTexture(std::shared_ptr<VulkanDeviceHandle> device, TextureDesc desc, VkImage image,
                             VmaAllocation allocation)
    : m_desc(std::move(desc)), m_image(image), m_allocation(allocation), m_device(std::move(device)) {
}

VulkanTexture::~VulkanTexture() {
    vmaDestroyImage(m_device->m_allocator, m_image, m_allocation);
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

    VkImageAspectFlags aspect = aspectsOf(texture_desc.format);
    if (desc.aspect == TextureAspect::DepthOnly) {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else if (desc.aspect == TextureAspect::StencilOnly) {
        aspect = VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    const std::uint32_t layer_count = desc.range.array_layer_count == kRemaining
                                          ? texture_desc.array_layer_count - desc.range.base_array_layer
                                          : desc.range.array_layer_count;

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = texture.handle();
    view_info.viewType = toVkViewType(texture_desc, desc.dimension, layer_count);
    view_info.format = toVkFormat(desc.format.value_or(texture_desc.format));
    // kRemaining equals VK_REMAINING_MIP_LEVELS / VK_REMAINING_ARRAY_LAYERS (~0u), so the range maps directly.
    view_info.subresourceRange = {aspect, desc.range.base_mip_level, desc.range.mip_level_count,
                                  desc.range.base_array_layer, desc.range.array_layer_count};

    VkImageView image_view = VK_NULL_HANDLE;
    if (VkResult result = vkCreateImageView(device->m_device, &view_info, nullptr, &image_view); result != VK_SUCCESS) {
        return toError(result, "vkCreateImageView");
    }
    return TextureViewRef(std::make_shared<VulkanTextureView>(std::move(device), std::move(desc), image_view));
}

VulkanTextureView::VulkanTextureView(std::shared_ptr<VulkanDeviceHandle> device, TextureViewDesc desc,
                                     VkImageView image_view)
    : m_desc(std::move(desc)), m_image_view(image_view), m_device(std::move(device)) {
}

VulkanTextureView::~VulkanTextureView() {
    vkDestroyImageView(m_device->m_device, m_image_view, nullptr);
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

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = toVkFilter(desc.mag_filter);
    sampler_info.minFilter = toVkFilter(desc.min_filter);
    sampler_info.mipmapMode =
        desc.mipmap_filter == Filter::Linear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = toVkAddressMode(desc.address_u);
    sampler_info.addressModeV = toVkAddressMode(desc.address_v);
    sampler_info.addressModeW = toVkAddressMode(desc.address_w);
    sampler_info.anisotropyEnable = anisotropy ? VK_TRUE : VK_FALSE;
    sampler_info.maxAnisotropy = anisotropy ? desc.max_anisotropy : 1.0f;
    sampler_info.compareEnable = desc.compare ? VK_TRUE : VK_FALSE;
    sampler_info.compareOp = desc.compare ? toVkCompareOp(*desc.compare) : VK_COMPARE_OP_NEVER;
    sampler_info.minLod = desc.min_lod;
    sampler_info.maxLod = desc.max_lod;

    VkSampler sampler = VK_NULL_HANDLE;
    if (VkResult result = vkCreateSampler(device->m_device, &sampler_info, nullptr, &sampler); result != VK_SUCCESS) {
        return toError(result, "vkCreateSampler");
    }
    return SamplerRef(std::make_shared<VulkanSampler>(std::move(device), std::move(desc), sampler));
}

VulkanSampler::VulkanSampler(std::shared_ptr<VulkanDeviceHandle> device, SamplerDesc desc, VkSampler sampler)
    : m_desc(std::move(desc)), m_sampler(sampler), m_device(std::move(device)) {
}

VulkanSampler::~VulkanSampler() {
    vkDestroySampler(m_device->m_device, m_sampler, nullptr);
}

SamplerDesc VulkanSampler::getDesc() const {
    return m_desc;
}

} // namespace rhi::vulkan
