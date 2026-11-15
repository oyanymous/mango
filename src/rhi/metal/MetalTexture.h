#pragma once

#include <memory>

#include "ISampler.h"
#include "ITexture.h"
#include "ITextureView.h"
#include "MetalCommon.h"

namespace rhi::metal {

class MetalTexture final : public ITexture {
public:
    static Result<TextureRef> create(std::shared_ptr<MetalDeviceContext> context, TextureDesc desc);

    MetalTexture(std::shared_ptr<MetalDeviceContext> context, TextureDesc desc, id<MTLTexture> texture);

    TextureDesc getDesc() const override;

    id<MTLTexture> handle() const { return m_texture; }

    void setHandle(id<MTLTexture> texture) { m_texture = texture; }
private:
    TextureDesc m_desc;
    id<MTLTexture> m_texture = nil;
    std::shared_ptr<MetalDeviceContext> m_context;
};

class MetalTextureView final : public ITextureView {
public:
    static Result<TextureViewRef> create(std::shared_ptr<MetalDeviceContext> context, TextureViewDesc desc);

    MetalTextureView(std::shared_ptr<MetalDeviceContext> context, TextureViewDesc desc, id<MTLTexture> view);

    TextureRef getTexture() const override;

    id<MTLTexture> handle() const;
private:
    TextureViewDesc m_desc;
    id<MTLTexture> m_view = nil;
    std::shared_ptr<MetalDeviceContext> m_context;
};

class MetalSampler final : public ISampler {
public:
    static Result<SamplerRef> create(std::shared_ptr<MetalDeviceContext> context, SamplerDesc desc,
                                     bool anisotropy_enabled);

    MetalSampler(std::shared_ptr<MetalDeviceContext> context, SamplerDesc desc, id<MTLSamplerState> sampler);

    SamplerDesc getDesc() const override;

    id<MTLSamplerState> handle() const { return m_sampler; }
private:
    SamplerDesc m_desc;
    id<MTLSamplerState> m_sampler = nil;
    std::shared_ptr<MetalDeviceContext> m_context;
};

} // namespace rhi::metal
