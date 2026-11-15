#pragma once

#include <memory>

#include "ISampler.h"
#include "ITexture.h"
#include "ITextureView.h"
#include "MetalCommon.h"

namespace rhi::metal {

/**
 * An `MTLTexture` in private storage.
 *
 * Metal has no image layouts and tracks hazards itself, so barriers record nothing for textures. A swapchain
 * backbuffer is a MetalTexture whose `MTLTexture` the swapchain replaces on every acquire (setHandle()), so always
 * read handle() at the time of use.
 */
class MetalTexture final : public ITexture {
public:
    /**
     * Creates a texture.
     *
     * @return The texture, InvalidArgument for a missing format/usage/extent or a cube without 6n layers, Unsupported
     *         for a format the GPU lacks (e.g. D24UnormS8Uint on Apple GPUs), or OutOfDeviceMemory.
     */
    static Result<TextureRef> create(std::shared_ptr<MetalDeviceContext> context, TextureDesc desc);

    /**
     * Wraps a texture. Use create(), or the swapchain for backbuffers.
     *
     * @param texture The texture; nil for a backbuffer before its first acquire.
     */
    MetalTexture(std::shared_ptr<MetalDeviceContext> context, TextureDesc desc, id<MTLTexture> texture);

    TextureDesc getDesc() const override;

    /// The current texture.
    id<MTLTexture> handle() const { return m_texture; }

    /**
     * Replaces the texture (swapchain backbuffers only).
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
 * A view of a MetalTexture.
 *
 * A view of the whole texture in its own format and natural dimension uses the texture itself (handle() follows a
 * backbuffer's current drawable); any other view is an `MTLTexture` view made with
 * `newTextureViewWithPixelFormat:textureType:levels:slices:`.
 */
class MetalTextureView final : public ITextureView {
public:
    /**
     * Creates a view.
     *
     * @return The view, or InvalidArgument for a null texture or a range outside it, or Unsupported for a format
     *         that changes the texel layout (Metal needs `MTLTextureUsagePixelFormatView` for that; only sRGB/linear
     *         pairs are allowed).
     */
    static Result<TextureViewRef> create(std::shared_ptr<MetalDeviceContext> context, TextureViewDesc desc);

    /**
     * Wraps a view. Use create().
     *
     * @param desc The descriptor it was created with (retains the texture).
     * @param view The view texture, or nil to use the texture itself.
     */
    MetalTextureView(std::shared_ptr<MetalDeviceContext> context, TextureViewDesc desc, id<MTLTexture> view);

    TextureRef getTexture() const override;

    /// The texture to bind or render into: the view, or the texture's current handle for a whole-texture view.
    id<MTLTexture> handle() const;
private:
    TextureViewDesc m_desc;
    id<MTLTexture> m_view = nil;
    std::shared_ptr<MetalDeviceContext> m_context;
};

/// An `MTLSamplerState`.
class MetalSampler final : public ISampler {
public:
    /**
     * Creates a sampler.
     *
     * @param anisotropy_enabled Whether the device was created with DeviceFeatures::sampler_anisotropy.
     * @return The sampler, or Unsupported for anisotropy on a device without the feature.
     */
    static Result<SamplerRef> create(std::shared_ptr<MetalDeviceContext> context, SamplerDesc desc,
                                     bool anisotropy_enabled);

    /// Wraps a sampler state. Use create().
    MetalSampler(std::shared_ptr<MetalDeviceContext> context, SamplerDesc desc, id<MTLSamplerState> sampler);

    SamplerDesc getDesc() const override;

    id<MTLSamplerState> handle() const { return m_sampler; }
private:
    SamplerDesc m_desc;
    id<MTLSamplerState> m_sampler = nil;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
