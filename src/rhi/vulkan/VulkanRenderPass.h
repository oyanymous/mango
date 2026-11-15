#pragma once

#include <memory>

#include "IRenderPass.h"
#include "IRenderTarget.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/**
 * The Vulkan IRenderPass: a single-subpass VkRenderPass. Created by VulkanDevice::createRenderPass.
 *
 * Attachments start and end in their attachment layout (COLOR_ATTACHMENT_OPTIMAL /
 * DEPTH_STENCIL_ATTACHMENT_OPTIMAL), matching the RenderTarget / DepthStencilWrite states that
 * RenderPassDesc requires around the pass.
 */
class VulkanRenderPass final : public IRenderPass {
public:
    /**
     * Creates the render pass.
     *
     * @return The pass, or InvalidArgument for no attachments or an Undefined format.
     */
    static Result<RenderPassRef> create(std::shared_ptr<VulkanDeviceHandle> device, RenderPassDesc desc);

    /// Takes ownership of a created render pass. Use create().
    VulkanRenderPass(std::shared_ptr<VulkanDeviceHandle> device, RenderPassDesc desc, VkRenderPass render_pass);
    ~VulkanRenderPass() override;

    VulkanRenderPass(const VulkanRenderPass&) = delete;
    VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;

    RenderPassDesc getDesc() const override;

    /// The VkRenderPass, for framebuffers, pipelines and vkCmdBeginRenderPass.
    VkRenderPass handle() const { return m_render_pass; }
private:
    RenderPassDesc m_desc;
    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

/// The Vulkan IRenderTarget: a VkFramebuffer. Created by VulkanDevice::createRenderTarget.
class VulkanRenderTarget final : public IRenderTarget {
public:
    /**
     * Creates the framebuffer.
     *
     * @return The target, or InvalidArgument when the views do not match the pass's attachments or the
     *         extent is zero.
     */
    static Result<RenderTargetRef> create(std::shared_ptr<VulkanDeviceHandle> device, RenderTargetDesc desc);

    /**
     * Takes ownership of a created framebuffer. Use create().
     *
     * @param desc The descriptor it was created with (retains the pass and views).
     */
    VulkanRenderTarget(std::shared_ptr<VulkanDeviceHandle> device, RenderTargetDesc desc, VkFramebuffer framebuffer);
    ~VulkanRenderTarget() override;

    VulkanRenderTarget(const VulkanRenderTarget&) = delete;
    VulkanRenderTarget& operator=(const VulkanRenderTarget&) = delete;

    RenderTargetDesc getDesc() const override;

    /// The VkFramebuffer, for vkCmdBeginRenderPass.
    VkFramebuffer handle() const { return m_framebuffer; }
private:
    RenderTargetDesc m_desc;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
