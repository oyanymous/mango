#include "VulkanRenderPass.h"

#include <vector>

#include "VulkanDevice.h"
#include "VulkanTexture.h"

namespace rhi::vulkan {

namespace {

vk::AttachmentLoadOp toVkLoadOp(LoadOp op) {
    switch (op) {
    case LoadOp::Load:
        return vk::AttachmentLoadOp::eLoad;
    case LoadOp::Clear:
        return vk::AttachmentLoadOp::eClear;
    case LoadOp::DontCare:
        return vk::AttachmentLoadOp::eDontCare;
    }
    return vk::AttachmentLoadOp::eDontCare;
}

vk::AttachmentStoreOp toVkStoreOp(StoreOp op) {
    return op == StoreOp::Store ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare;
}

vk::AttachmentDescription toVkAttachment(const AttachmentDesc& desc, vk::ImageLayout layout) {
    return vk::AttachmentDescription{
        .format = toVkFormat(desc.format),
        .samples = static_cast<vk::SampleCountFlagBits>(desc.sample_count),
        .loadOp = toVkLoadOp(desc.load_op),
        .storeOp = toVkStoreOp(desc.store_op),
        .stencilLoadOp = toVkLoadOp(desc.stencil_load_op),
        .stencilStoreOp = toVkStoreOp(desc.stencil_store_op),
        .initialLayout = layout,
        .finalLayout = layout,
    };
}

} // namespace

Result<RenderPassRef> VulkanRenderPass::create(std::shared_ptr<VulkanDeviceHandle> device, RenderPassDesc desc) {
    if (desc.color_attachments.empty() && !desc.depth_stencil_attachment) {
        return Error{ErrorCode::InvalidArgument, "createRenderPass: the pass has no attachments"};
    }

    std::vector<vk::AttachmentDescription> attachments;
    std::vector<vk::AttachmentReference> color_refs;
    for (const AttachmentDesc& color : desc.color_attachments) {
        if (color.format == Format::Undefined) {
            return Error{ErrorCode::InvalidArgument, "createRenderPass: colour attachment format is Undefined"};
        }
        color_refs.push_back({
            .attachment = static_cast<std::uint32_t>(attachments.size()),
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
        });
        attachments.push_back(toVkAttachment(color, vk::ImageLayout::eColorAttachmentOptimal));
    }
    vk::AttachmentReference depth_ref{};
    if (desc.depth_stencil_attachment) {
        if (desc.depth_stencil_attachment->format == Format::Undefined) {
            return Error{ErrorCode::InvalidArgument, "createRenderPass: depth attachment format is Undefined"};
        }
        depth_ref = {
            .attachment = static_cast<std::uint32_t>(attachments.size()),
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        };
        attachments.push_back(
            toVkAttachment(*desc.depth_stencil_attachment, vk::ImageLayout::eDepthStencilAttachmentOptimal));
    }

    const vk::SubpassDescription subpass{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = static_cast<std::uint32_t>(color_refs.size()),
        .pColorAttachments = color_refs.data(),
        .pDepthStencilAttachment = desc.depth_stencil_attachment ? &depth_ref : nullptr,
    };
    const vk::RenderPassCreateInfo pass_info{
        .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    Result<vk::raii::RenderPass> render_pass =
        check(device->m_device.createRenderPass(pass_info, nullptr), "vkCreateRenderPass");
    if (!render_pass) {
        return render_pass.error();
    }
    return RenderPassRef(
        std::make_shared<VulkanRenderPass>(std::move(device), std::move(desc), std::move(render_pass).value()));
}

VulkanRenderPass::VulkanRenderPass(std::shared_ptr<VulkanDeviceHandle> device, RenderPassDesc desc,
                                   vk::raii::RenderPass render_pass)
    : m_device(std::move(device)), m_desc(std::move(desc)), m_render_pass(std::move(render_pass)) {
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

    std::vector<vk::ImageView> views;
    for (const TextureViewRef& view : desc.color_attachments) {
        if (!view) {
            return Error{ErrorCode::InvalidArgument, "createRenderTarget: null colour attachment"};
        }
        views.push_back(static_cast<const VulkanTextureView&>(*view).handle());
    }
    if (desc.depth_stencil_attachment) {
        views.push_back(static_cast<const VulkanTextureView&>(*desc.depth_stencil_attachment).handle());
    }

    const vk::FramebufferCreateInfo framebuffer_info{
        .renderPass = static_cast<const VulkanRenderPass&>(*desc.render_pass).handle(),
        .attachmentCount = static_cast<std::uint32_t>(views.size()),
        .pAttachments = views.data(),
        .width = desc.extent.width,
        .height = desc.extent.height,
        .layers = 1,
    };
    Result<vk::raii::Framebuffer> framebuffer =
        check(device->m_device.createFramebuffer(framebuffer_info, nullptr), "vkCreateFramebuffer");
    if (!framebuffer) {
        return framebuffer.error();
    }
    return RenderTargetRef(
        std::make_shared<VulkanRenderTarget>(std::move(device), std::move(desc), std::move(framebuffer).value()));
}

VulkanRenderTarget::VulkanRenderTarget(std::shared_ptr<VulkanDeviceHandle> device, RenderTargetDesc desc,
                                       vk::raii::Framebuffer framebuffer)
    : m_device(std::move(device)), m_desc(std::move(desc)), m_framebuffer(std::move(framebuffer)) {
}

RenderTargetDesc VulkanRenderTarget::getDesc() const {
    return m_desc;
}

} // namespace rhi::vulkan
