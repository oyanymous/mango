#pragma once

#include <memory>

#include "IRenderPass.h"
#include "IRenderTarget.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

class VulkanRenderPass final : public IRenderPass {
public:
    static Result<RenderPassRef> create(std::shared_ptr<VulkanDeviceHandle> device, RenderPassDesc desc);

    VulkanRenderPass(std::shared_ptr<VulkanDeviceHandle> device, RenderPassDesc desc, vk::raii::RenderPass render_pass);

    VulkanRenderPass(const VulkanRenderPass&) = delete;
    VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;

    RenderPassDesc getDesc() const override;

    vk::RenderPass handle() const { return *m_render_pass; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    RenderPassDesc m_desc;
    vk::raii::RenderPass m_render_pass;
};

class VulkanRenderTarget final : public IRenderTarget {
public:
    static Result<RenderTargetRef> create(std::shared_ptr<VulkanDeviceHandle> device, RenderTargetDesc desc);

    VulkanRenderTarget(std::shared_ptr<VulkanDeviceHandle> device, RenderTargetDesc desc,
                       vk::raii::Framebuffer framebuffer);

    VulkanRenderTarget(const VulkanRenderTarget&) = delete;
    VulkanRenderTarget& operator=(const VulkanRenderTarget&) = delete;

    RenderTargetDesc getDesc() const override;

    vk::Framebuffer handle() const { return *m_framebuffer; }
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    RenderTargetDesc m_desc;
    vk::raii::Framebuffer m_framebuffer;
};

} // namespace rhi::vulkan
