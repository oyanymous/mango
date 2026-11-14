#pragma once

/**
 * @file
 * @brief rhi::vulkan::VulkanRenderPass and VulkanRenderTarget: a VkRenderPass and a VkFramebuffer.
 */

#include <memory>

#include "IRenderPass.h"
#include "IRenderTarget.h"
#include "Result.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/**
 * @brief The Vulkan IRenderPass: a single-subpass VkRenderPass. Created by VulkanDevice::createRenderPass.
 *
 * @details
 * Attachments start and end in their attachment layout (COLOR_ATTACHMENT_OPTIMAL /
 * DEPTH_STENCIL_ATTACHMENT_OPTIMAL), matching the RenderTarget / DepthStencilWrite states that
 * RenderPassDesc requires around the pass.
 */
class VulkanRenderPass final : public IRenderPass {
public:
    /**
     * @brief Creates the render pass.
     *
     * @param device Owner of the VkDevice; kept alive by the pass.
     * @param desc Attachment formats and load/store ops.
     * @return The pass, or InvalidArgument for no attachments or an Undefined format.
     */
    static Result<RenderPassRef> create(std::shared_ptr<VulkanDeviceHandle> device, RenderPassDesc desc);

    /**
     * @brief Takes ownership of a created render pass. Use create().
     *
     * @param device Owner of the VkDevice.
     * @param desc The descriptor it was created with.
     * @param render_pass The render pass.
     */
    VulkanRenderPass(std::shared_ptr<VulkanDeviceHandle> device, RenderPassDesc desc, VkRenderPass render_pass);
    /**
     * @brief Destroys the render pass.
     */
    ~VulkanRenderPass() override;

    VulkanRenderPass(const VulkanRenderPass&) = delete;
    VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;

    RenderPassDesc getDesc() const override;

    /**
     * @brief The VkRenderPass, for framebuffers, pipelines and vkCmdBeginRenderPass.
     */
    VkRenderPass handle() const { return m_render_pass; }
private:
    RenderPassDesc m_desc;
    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

/**
 * @brief The Vulkan IRenderTarget: a VkFramebuffer. Created by VulkanDevice::createRenderTarget.
 */
class VulkanRenderTarget final : public IRenderTarget {
public:
    /**
     * @brief Creates the framebuffer.
     *
     * @param device Owner of the VkDevice; kept alive by the target.
     * @param desc Pass, attachment views and extent.
     * @return The target, or InvalidArgument when the views do not match the pass's attachments or the
     *         extent is zero.
     */
    static Result<RenderTargetRef> create(std::shared_ptr<VulkanDeviceHandle> device, RenderTargetDesc desc);

    /**
     * @brief Takes ownership of a created framebuffer. Use create().
     *
     * @param device Owner of the VkDevice.
     * @param desc The descriptor it was created with (retains the pass and views).
     * @param framebuffer The framebuffer.
     */
    VulkanRenderTarget(std::shared_ptr<VulkanDeviceHandle> device, RenderTargetDesc desc, VkFramebuffer framebuffer);
    /**
     * @brief Destroys the framebuffer.
     */
    ~VulkanRenderTarget() override;

    VulkanRenderTarget(const VulkanRenderTarget&) = delete;
    VulkanRenderTarget& operator=(const VulkanRenderTarget&) = delete;

    RenderTargetDesc getDesc() const override;

    /**
     * @brief The VkFramebuffer, for vkCmdBeginRenderPass.
     */
    VkFramebuffer handle() const { return m_framebuffer; }
private:
    RenderTargetDesc m_desc;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
