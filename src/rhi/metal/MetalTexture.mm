#include "MetalTexture.h"

#include <utility>

namespace rhi::metal {

namespace {

MTLTextureType textureType(const TextureDesc& desc) {
    switch (desc.dimension) {
    case TextureDimension::Texture2D:
        if (desc.sample_count > 1) {
            return desc.array_layer_count > 1 ? MTLTextureType2DMultisampleArray : MTLTextureType2DMultisample;
        }
        return desc.array_layer_count > 1 ? MTLTextureType2DArray : MTLTextureType2D;
    case TextureDimension::Texture3D:
        return MTLTextureType3D;
    case TextureDimension::TextureCube:
        return desc.array_layer_count > 6 ? MTLTextureTypeCubeArray : MTLTextureTypeCube;
    }
    return MTLTextureType2D;
}

MTLTextureType viewType(const TextureDesc& texture, std::optional<TextureViewDimension> dimension,
                        std::uint32_t layer_count) {
    if (!dimension) {
        if (texture.dimension == TextureDimension::Texture2D && layer_count == 1) {
            return texture.sample_count > 1 ? MTLTextureType2DMultisample : MTLTextureType2D;
        }
        if (texture.dimension == TextureDimension::TextureCube && layer_count == 6) {
            return MTLTextureTypeCube;
        }
        return textureType(texture);
    }
    switch (*dimension) {
    case TextureViewDimension::Texture2D:
        return texture.sample_count > 1 ? MTLTextureType2DMultisample : MTLTextureType2D;
    case TextureViewDimension::Texture2DArray:
        return texture.sample_count > 1 ? MTLTextureType2DMultisampleArray : MTLTextureType2DArray;
    case TextureViewDimension::Texture3D:
        return MTLTextureType3D;
    case TextureViewDimension::TextureCube:
        return layer_count > 6 ? MTLTextureTypeCubeArray : MTLTextureTypeCube;
    }
    return MTLTextureType2D;
}

bool isCompatibleViewFormat(Format from, Format to) {
    if (from == to) {
        return true;
    }
    const auto same_pair = [&](Format a, Format b) { return (from == a && to == b) || (from == b && to == a); };
    return same_pair(Format::RGBA8Unorm, Format::RGBA8Srgb) || same_pair(Format::BGRA8Unorm, Format::BGRA8Srgb);
}

MTLSamplerAddressMode toMTLAddressMode(AddressMode mode) {
    switch (mode) {
    case AddressMode::Repeat:
        return MTLSamplerAddressModeRepeat;
    case AddressMode::MirroredRepeat:
        return MTLSamplerAddressModeMirrorRepeat;
    case AddressMode::ClampToEdge:
        return MTLSamplerAddressModeClampToEdge;
    }
    return MTLSamplerAddressModeRepeat;
}

} // namespace

Result<TextureRef> MetalTexture::create(std::shared_ptr<MetalDeviceContext> context, TextureDesc desc) {
    if (desc.format == Format::Undefined || desc.usage == TextureUsage::None || desc.extent.width == 0 ||
        desc.extent.height == 0 || desc.extent.depth == 0 || desc.mip_level_count == 0 || desc.array_layer_count == 0) {
        return Error{ErrorCode::InvalidArgument, "createTexture: format, usage, extent and counts must be set"};
    }
    const bool cube = desc.dimension == TextureDimension::TextureCube;
    if (cube && desc.array_layer_count % 6 != 0) {
        return Error{ErrorCode::InvalidArgument, "createTexture: a cube needs a multiple of 6 layers"};
    }
    const MTLPixelFormat format = toMTLPixelFormat(desc.format);
    if (format == MTLPixelFormatInvalid ||
        (desc.format == Format::D24UnormS8Uint && !context->device.depth24Stencil8PixelFormatSupported)) {
        return Error{ErrorCode::Unsupported, "createTexture: the GPU does not support this format"};
    }

    MTLTextureDescriptor* descriptor = [[MTLTextureDescriptor alloc] init];
    descriptor.textureType = textureType(desc);
    descriptor.pixelFormat = format;
    descriptor.width = desc.extent.width;
    descriptor.height = desc.extent.height;
    descriptor.depth = desc.dimension == TextureDimension::Texture3D ? desc.extent.depth : 1;
    descriptor.mipmapLevelCount = desc.mip_level_count;
    descriptor.arrayLength = cube ? desc.array_layer_count / 6 : desc.array_layer_count;
    descriptor.sampleCount = desc.sample_count;
    descriptor.storageMode = MTLStorageModePrivate;
    MTLTextureUsage usage = MTLTextureUsageUnknown;
    if (hasFlag(desc.usage, TextureUsage::Sampled)) {
        usage |= MTLTextureUsageShaderRead;
    }
    if (hasFlag(desc.usage, TextureUsage::Storage)) {
        usage |= MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    }
    if (hasFlag(desc.usage, TextureUsage::RenderTarget) || hasFlag(desc.usage, TextureUsage::DepthStencil)) {
        usage |= MTLTextureUsageRenderTarget;
    }
    descriptor.usage = usage;

    id<MTLTexture> texture = [context->device newTextureWithDescriptor:descriptor];
    if (texture == nil) {
        return Error{ErrorCode::OutOfDeviceMemory, "MTLDevice newTextureWithDescriptor failed"};
    }
    texture.label = toNSString(desc.label);
    return TextureRef(std::make_shared<MetalTexture>(std::move(context), std::move(desc), texture));
}

MetalTexture::MetalTexture(std::shared_ptr<MetalDeviceContext> context, TextureDesc desc, id<MTLTexture> texture)
    : m_desc(std::move(desc)), m_texture(texture), m_context(std::move(context)) {
}

TextureDesc MetalTexture::getDesc() const {
    return m_desc;
}

Result<TextureViewRef> MetalTextureView::create(std::shared_ptr<MetalDeviceContext> context, TextureViewDesc desc) {
    if (!desc.texture) {
        return Error{ErrorCode::InvalidArgument, "createTextureView: texture is null"};
    }
    const auto& texture = static_cast<const MetalTexture&>(*desc.texture);
    const TextureDesc texture_desc = texture.getDesc();

    const TextureSubresourceRange& range = desc.range;
    if (range.base_mip_level >= texture_desc.mip_level_count ||
        range.base_array_layer >= texture_desc.array_layer_count) {
        return Error{ErrorCode::InvalidArgument, "createTextureView: range starts outside the texture"};
    }
    const std::uint32_t levels = range.mip_level_count == kRemaining
                                     ? texture_desc.mip_level_count - range.base_mip_level
                                     : range.mip_level_count;
    const std::uint32_t layers = range.array_layer_count == kRemaining
                                     ? texture_desc.array_layer_count - range.base_array_layer
                                     : range.array_layer_count;
    if (levels == 0 || layers == 0 || range.base_mip_level + levels > texture_desc.mip_level_count ||
        range.base_array_layer + layers > texture_desc.array_layer_count) {
        return Error{ErrorCode::InvalidArgument, "createTextureView: range is outside the texture"};
    }

    Format format = desc.format.value_or(texture_desc.format);
    if (!isCompatibleViewFormat(texture_desc.format, format)) {
        return Error{ErrorCode::Unsupported, "createTextureView: Metal cannot reinterpret this texture's format"};
    }
    MTLPixelFormat pixel_format = toMTLPixelFormat(format);

    if (desc.aspect == TextureAspect::StencilOnly && hasStencil(format)) {
        pixel_format = format == Format::D32FloatS8Uint ? MTLPixelFormatX32_Stencil8 : MTLPixelFormatX24_Stencil8;
    }

    const MTLTextureType type = viewType(texture_desc, desc.dimension, layers);
    const bool whole = pixel_format == toMTLPixelFormat(texture_desc.format) && type == textureType(texture_desc) &&
                       levels == texture_desc.mip_level_count && layers == texture_desc.array_layer_count;
    id<MTLTexture> view = nil;
    if (!whole) {
        view = [texture.handle() newTextureViewWithPixelFormat:pixel_format
                                                   textureType:type
                                                        levels:NSMakeRange(range.base_mip_level, levels)
                                                        slices:NSMakeRange(range.base_array_layer, layers)];
        if (view == nil) {
            return Error{ErrorCode::InvalidArgument, "MTLTexture newTextureViewWithPixelFormat failed"};
        }
        view.label = toNSString(desc.label);
    }
    return TextureViewRef(std::make_shared<MetalTextureView>(std::move(context), std::move(desc), view));
}

MetalTextureView::MetalTextureView(std::shared_ptr<MetalDeviceContext> context, TextureViewDesc desc,
                                   id<MTLTexture> view)
    : m_desc(std::move(desc)), m_view(view), m_context(std::move(context)) {
}

TextureRef MetalTextureView::getTexture() const {
    return m_desc.texture;
}

id<MTLTexture> MetalTextureView::handle() const {
    return m_view != nil ? m_view : static_cast<const MetalTexture&>(*m_desc.texture).handle();
}

Result<SamplerRef> MetalSampler::create(std::shared_ptr<MetalDeviceContext> context, SamplerDesc desc,
                                        bool anisotropy_enabled) {
    const bool anisotropy = desc.max_anisotropy > 1.0f;
    if (anisotropy && !anisotropy_enabled) {
        return Error{ErrorCode::Unsupported,
                     "createSampler: max_anisotropy > 1 needs DeviceFeatures::sampler_anisotropy"};
    }

    MTLSamplerDescriptor* descriptor = [[MTLSamplerDescriptor alloc] init];
    descriptor.minFilter =
        desc.min_filter == Filter::Linear ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
    descriptor.magFilter =
        desc.mag_filter == Filter::Linear ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
    descriptor.mipFilter =
        desc.mipmap_filter == Filter::Linear ? MTLSamplerMipFilterLinear : MTLSamplerMipFilterNearest;
    descriptor.sAddressMode = toMTLAddressMode(desc.address_u);
    descriptor.tAddressMode = toMTLAddressMode(desc.address_v);
    descriptor.rAddressMode = toMTLAddressMode(desc.address_w);
    descriptor.lodMinClamp = desc.min_lod;
    descriptor.lodMaxClamp = desc.max_lod;
    descriptor.maxAnisotropy = anisotropy ? static_cast<NSUInteger>(desc.max_anisotropy) : 1;
    descriptor.compareFunction = desc.compare ? toMTLCompareFunction(*desc.compare) : MTLCompareFunctionNever;

    if (!desc.label.empty()) {
        descriptor.label = toNSString(desc.label);
    }

    id<MTLSamplerState> sampler = [context->device newSamplerStateWithDescriptor:descriptor];
    if (sampler == nil) {
        return Error{ErrorCode::InitializationFailed, "MTLDevice newSamplerStateWithDescriptor failed"};
    }
    return SamplerRef(std::make_shared<MetalSampler>(std::move(context), std::move(desc), sampler));
}

MetalSampler::MetalSampler(std::shared_ptr<MetalDeviceContext> context, SamplerDesc desc, id<MTLSamplerState> sampler)
    : m_desc(std::move(desc)), m_sampler(sampler), m_context(std::move(context)) {
}

SamplerDesc MetalSampler::getDesc() const {
    return m_desc;
}

} // namespace rhi::metal
