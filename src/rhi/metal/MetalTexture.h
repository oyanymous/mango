#pragma once

/**
 * @file
 * @brief rhi::metal::MetalTexture, MetalTextureView and MetalSampler.
 */

#include <memory>

#include "ISampler.h"
#include "ITexture.h"
#include "ITextureView.h"
#include "MetalCommon.h"

namespace rhi::metal {

/**
 * @brief An `MTLTexture` in private storage.
 *
 * @details
 * Metal has no image layouts and tracks hazards itself, so barriers record nothing for textures. A swapchain
 * backbuffer is a MetalTexture whose `MTLTexture` the swapchain replaces on every acquire (setHandle()), so always
 * read handle() at the time of use.
 */
class MetalTexture final : public ITexture {
public:
    /**
     * @brief Creates a texture.
     *
     * @param context Device context; kept alive by the texture.
     * @param desc Shape, format, usage and label.
     * @return The texture, InvalidArgument for a missing format/usage/extent or a cube without 6n layers, Unsupported
     *         for a format the GPU lacks (e.g. D24UnormS8Uint on Apple GPUs), or OutOfDeviceMemory.
     */
    static Result<TextureRef> create(std::shared_ptr<MetalDeviceContext> context, TextureDesc desc);

    /**
     * @brief Wraps a texture. Use create(), or the swapchain for backbuffers.
     *
     * @param context Device context.
     * @param desc The descriptor it was created with.
     * @param texture The texture; nil for a backbuffer before its first acquire.
     */
    MetalTexture(std::shared_ptr<MetalDeviceContext> context, TextureDesc desc, id<MTLTexture> texture);

    TextureDesc getDesc() const override;

    /**
     * @brief The current texture.
     */
    id<MTLTexture> handle() const { return m_texture; }

    /**
     * @brief Replaces the texture (swapchain backbuffers only).
     *
     * @param texture The drawable's texture.
     */
    void setHandle(id<MTLTexture> texture) { m_texture = texture; }
private:
    TextureDesc m_desc;
    id<MTLTexture> m_texture = nil;
    std::shared_ptr<MetalDeviceContext> m_context;
};

/**
 * @brief A view of a MetalTexture.
 *
 * @details
 * A view of the whole texture in its own format and natural dimension uses the texture itself (handle() follows a
 * backbuffer's current drawable); any other view is an `MTLTexture` view made with
 * `newTextureViewWithPixelFormat:textureType:levels:slices:`.
 */
class MetalTextureView final : public ITextureView {
public:
    /**
     * @brief Creates a view.
     *
     * @param context Device context; kept alive by the view.
     * @param desc Texture, format, dimension, aspect and range.
     * @return The view, or InvalidArgument for a null texture or a range outside it, or Unsupported for a format
     *         that changes the texel layout (Metal needs `MTLTextureUsagePixelFormatView` for that; only sRGB/linear
     *         pairs are allowed).
     */
    static Result<TextureViewRef> create(std::shared_ptr<MetalDeviceContext> context, TextureViewDesc desc);

    /**
     * @brief Wraps a view. Use create().
     *
     * @param context Device context.
     * @param desc The descriptor it was created with (retains the texture).
     * @param view The view texture, or nil to use the texture itself.
     */
    MetalTextureView(std::shared_ptr<MetalDeviceContext> context, TextureViewDesc desc, id<MTLTexture> view);

    TextureRef getTexture() const override;

    /**
     * @brief The texture to bind or render into: the view, or the texture's current handle for a whole-texture view.
     */
    id<MTLTexture> handle() const;
private:
    TextureViewDesc m_desc;
    id<MTLTexture> m_view = nil;
    std::shared_ptr<MetalDeviceContext> m_context;
};

/**
 * @brief An `MTLSamplerState`.
 */
class MetalSampler final : public ISampler {
public:
    /**
     * @brief Creates a sampler.
     *
     * @param context Device context; kept alive by the sampler.
     * @param desc Filters, addressing, LOD range, anisotropy and compare function.
     * @param anisotropy_enabled Whether the device was created with DeviceFeatures::sampler_anisotropy.
     * @return The sampler, or Unsupported for anisotropy on a device without the feature.
     */
    static Result<SamplerRef> create(std::shared_ptr<MetalDeviceContext> context, SamplerDesc desc,
                                     bool anisotropy_enabled);

    /**
     * @brief Wraps a sampler state. Use create().
     *
     * @param context Device context.
     * @param desc The descriptor it was created with.
     * @param sampler The sampler state.
     */
    MetalSampler(std::shared_ptr<MetalDeviceContext> context, SamplerDesc desc, id<MTLSamplerState> sampler);

    SamplerDesc getDesc() const override;

    /**
     * @brief The sampler state.
     */
    id<MTLSamplerState> handle() const { return m_sampler; }
private:
    SamplerDesc m_desc;
    id<MTLSamplerState> m_sampler = nil;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
