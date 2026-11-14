/**
 * @file
 * @brief VulkanRenderPass and VulkanRenderTarget.
 */

#include "VulkanRenderPass.h"

#include <vector>

#include "VulkanDevice.h"
#include "VulkanTexture.h"

namespace rhi::vulkan {

namespace {

VkAttachmentLoadOp toVkLoadOp(LoadOp op) {
    switch (op) {
    case LoadOp::Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case LoadOp::Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case LoadOp::DontCare:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

VkAttachmentStoreOp toVkStoreOp(StoreOp op) {
    return op == StoreOp::Store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

/**
 * @brief One attachment description; it stays in @p layout for the whole pass.
 */
VkAttachmentDescription toVkAttachment(const AttachmentDesc& desc, VkImageLayout layout) {
    VkAttachmentDescription attachment{};
    attachment.format = toVkFormat(desc.format);
    attachment.samples = static_cast<VkSampleCountFlagBits>(desc.sample_count);
    attachment.loadOp = toVkLoadOp(desc.load_op);
    attachment.storeOp = toVkStoreOp(desc.store_op);
    attachment.stencilLoadOp = toVkLoadOp(desc.stencil_load_op);
    attachment.stencilStoreOp = toVkStoreOp(desc.stencil_store_op);
    attachment.initialLayout = layout;
    attachment.finalLayout = layout;
    return attachment;
}

} // namespace

Result<RenderPassRef> VulkanRenderPass::create(std::shared_ptr<VulkanDeviceHandle> device, RenderPassDesc desc) {
    if (desc.color_attachments.empty() && !desc.depth_stencil_attachment) {
        return Error{ErrorCode::InvalidArgument, "createRenderPass: the pass has no attachments"};
    }

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> color_refs;
    for (const AttachmentDesc& color : desc.color_attachments) {
        if (color.format == Format::Undefined) {
            return Error{ErrorCode::InvalidArgument, "createRenderPass: colour attachment format is Undefined"};
        }
        color_refs.push_back(
            {static_cast<std::uint32_t>(attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        attachments.push_back(toVkAttachment(color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
    }
    VkAttachmentReference depth_ref{};
    if (desc.depth_stencil_attachment) {
        if (desc.depth_stencil_attachment->format == Format::Undefined) {
            return Error{ErrorCode::InvalidArgument, "createRenderPass: depth attachment format is Undefined"};
        }
        depth_ref = {static_cast<std::uint32_t>(attachments.size()), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        attachments.push_back(
            toVkAttachment(*desc.depth_stencil_attachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
    }

    // No explicit subpass dependencies: callers transition attachments with ICommandBuffer::barrier into and
    // out of the attachment states, and those barriers cover the load and store operations.
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<std::uint32_t>(color_refs.size());
    subpass.pColorAttachments = color_refs.data();
    subpass.pDepthStencilAttachment = desc.depth_stencil_attachment ? &depth_ref : nullptr;

    VkRenderPassCreateInfo pass_info{};
    pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass_info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    pass_info.pAttachments = attachments.data();
    pass_info.subpassCount = 1;
    pass_info.pSubpasses = &subpass;

    VkRenderPass render_pass = VK_NULL_HANDLE;
    if (VkResult result = vkCreateRenderPass(device->m_device, &pass_info, nullptr, &render_pass);
        result != VK_SUCCESS) {
        return toError(result, "vkCreateRenderPass");
    }
    return RenderPassRef(std::make_shared<VulkanRenderPass>(std::move(device), std::move(desc), render_pass));
}

VulkanRenderPass::VulkanRenderPass(std::shared_ptr<VulkanDeviceHandle> device, RenderPassDesc desc,
                                   VkRenderPass render_pass)
    : m_desc(std::move(desc)), m_render_pass(render_pass), m_device(std::move(device)) {
}

VulkanRenderPass::~VulkanRenderPass() {
    vkDestroyRenderPass(m_device->m_device, m_render_pass, nullptr);
}

RenderPassDesc VulkanRenderPass::getDesc() const {
    return m_desc;
}

Result<RenderTargetRef> VulkanRenderTarget::create(std::shared_ptr<VulkanDeviceHandle> device, RenderTargetDesc desc) {
    if (!desc.render_pass) {
        return Error{ErrorCode::InvalidArgument, "createRenderTarget: render_pass is null"};
    }
    const RenderPassDesc pass_desc = desc.render_pass->getDesc();
    if (desc.color_attachments.size() != pass_desc.color_attachments.size() ||
        static_cast<bool>(desc.depth_stencil_attachment) != pass_desc.depth_stencil_attachment.has_value()) {
        return Error{ErrorCode::InvalidArgument, "createRenderTarget: attachments do not match the render pass"};
    }
    if (desc.extent.width == 0 || desc.extent.height == 0) {
        return Error{ErrorCode::InvalidArgument, "createRenderTarget: extent must be non-zero"};
    }

    std::vector<VkImageView> views;
    for (const TextureViewRef& view : desc.color_attachments) {
        if (!view) {
            return Error{ErrorCode::InvalidArgument, "createRenderTarget: null colour attachment"};
        }
        views.push_back(static_cast<const VulkanTextureView&>(*view).handle());
    }
    if (desc.depth_stencil_attachment) {
        views.push_back(static_cast<const VulkanTextureView&>(*desc.depth_stencil_attachment).handle());
    }

    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = static_cast<const VulkanRenderPass&>(*desc.render_pass).handle();
    framebuffer_info.attachmentCount = static_cast<std::uint32_t>(views.size());
    framebuffer_info.pAttachments = views.data();
    framebuffer_info.width = desc.extent.width;
    framebuffer_info.height = desc.extent.height;
    framebuffer_info.layers = 1;

    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    if (VkResult result = vkCreateFramebuffer(device->m_device, &framebuffer_info, nullptr, &framebuffer);
        result != VK_SUCCESS) {
        return toError(result, "vkCreateFramebuffer");
    }
    return RenderTargetRef(std::make_shared<VulkanRenderTarget>(std::move(device), std::move(desc), framebuffer));
}

VulkanRenderTarget::VulkanRenderTarget(std::shared_ptr<VulkanDeviceHandle> device, RenderTargetDesc desc,
                                       VkFramebuffer framebuffer)
    : m_desc(std::move(desc)), m_framebuffer(framebuffer), m_device(std::move(device)) {
}

VulkanRenderTarget::~VulkanRenderTarget() {
    vkDestroyFramebuffer(m_device->m_device, m_framebuffer, nullptr);
}

RenderTargetDesc VulkanRenderTarget::getDesc() const {
    return m_desc;
}

} // namespace rhi::vulkan
