/**
 * @file
 * @brief VulkanCommandBuffer.
 */

#include "VulkanCommandBuffer.h"

#include "VulkanBuffer.h"
#include "VulkanComputeEncoder.h"
#include "VulkanCopyEncoder.h"
#include "VulkanDevice.h"
#include "VulkanRenderEncoder.h"
#include "VulkanRenderPass.h"
#include "VulkanSemaphore.h"
#include "VulkanTexture.h"

namespace rhi::vulkan {

namespace {

/// The pipeline stages and memory accesses a resource in a given state is used with.
struct StageAccess {
    vk::PipelineStageFlags stages;
    vk::AccessFlags access;
};

/// Maps a ResourceState to the stages and accesses that use it, for barriers.
StageAccess toStageAccess(ResourceState state) {
    using Stage = vk::PipelineStageFlagBits;
    using Access = vk::AccessFlagBits;
    constexpr vk::PipelineStageFlags kShaders = Stage::eVertexShader | Stage::eFragmentShader | Stage::eComputeShader;
    constexpr vk::PipelineStageFlags kFragmentTests = Stage::eEarlyFragmentTests | Stage::eLateFragmentTests;
    switch (state) {
    case ResourceState::Undefined:
        return {Stage::eTopOfPipe, {}};
    case ResourceState::CopySrc:
        return {Stage::eTransfer, Access::eTransferRead};
    case ResourceState::CopyDst:
        return {Stage::eTransfer, Access::eTransferWrite};
    case ResourceState::VertexBuffer:
        return {Stage::eVertexInput, Access::eVertexAttributeRead};
    case ResourceState::IndexBuffer:
        return {Stage::eVertexInput, Access::eIndexRead};
    case ResourceState::UniformBuffer:
        return {kShaders, Access::eUniformRead};
    case ResourceState::ShaderRead:
        return {kShaders, Access::eShaderRead};
    case ResourceState::ShaderWrite:
        return {kShaders, Access::eShaderRead | Access::eShaderWrite};
    case ResourceState::RenderTarget:
        return {Stage::eColorAttachmentOutput, Access::eColorAttachmentRead | Access::eColorAttachmentWrite};
    case ResourceState::DepthStencilWrite:
        return {kFragmentTests, Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite};
    case ResourceState::DepthStencilRead:
        return {kFragmentTests | kShaders, Access::eDepthStencilAttachmentRead | Access::eShaderRead};
    case ResourceState::Present:
        return {Stage::eBottomOfPipe, {}};
    }
    return {Stage::eAllCommands, Access::eMemoryRead | Access::eMemoryWrite};
}

/// The image layout a texture in a given state is in. States only buffers use map to GENERAL.
vk::ImageLayout toImageLayout(ResourceState state) {
    switch (state) {
    case ResourceState::Undefined:
        return vk::ImageLayout::eUndefined;
    case ResourceState::CopySrc:
        return vk::ImageLayout::eTransferSrcOptimal;
    case ResourceState::CopyDst:
        return vk::ImageLayout::eTransferDstOptimal;
    case ResourceState::ShaderRead:
        return vk::ImageLayout::eShaderReadOnlyOptimal;
    case ResourceState::RenderTarget:
        return vk::ImageLayout::eColorAttachmentOptimal;
    case ResourceState::DepthStencilWrite:
        return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    case ResourceState::DepthStencilRead:
        return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    case ResourceState::Present:
        return vk::ImageLayout::ePresentSrcKHR;
    case ResourceState::ShaderWrite:
    case ResourceState::VertexBuffer:
    case ResourceState::IndexBuffer:
    case ResourceState::UniformBuffer:
        return vk::ImageLayout::eGeneral;
    }
    return vk::ImageLayout::eGeneral;
}

} // namespace

Result<CommandBufferRef> VulkanCommandBuffer::create(std::shared_ptr<VulkanDeviceHandle> device, CommandBufferDesc desc,
                                                     std::uint32_t family_index) {
    Result<vk::raii::CommandPool> pool =
        check(device->m_device.createCommandPool({.queueFamilyIndex = family_index}, nullptr), "vkCreateCommandPool");
    if (!pool) {
        return pool.error();
    }
    Result<std::vector<vk::raii::CommandBuffer>> command_buffers = check(device->m_device.allocateCommandBuffers({
                                                                             .commandPool = *pool.value(),
                                                                             .level = vk::CommandBufferLevel::ePrimary,
                                                                             .commandBufferCount = 1,
                                                                         }),
                                                                         "vkAllocateCommandBuffers");
    if (!command_buffers) {
        return command_buffers.error();
    }
    return CommandBufferRef(std::make_shared<VulkanCommandBuffer>(
        std::move(device), std::move(desc), std::move(pool).value(), std::move(command_buffers.value().front())));
}

VulkanCommandBuffer::VulkanCommandBuffer(std::shared_ptr<VulkanDeviceHandle> device, CommandBufferDesc desc,
                                         vk::raii::CommandPool pool, vk::raii::CommandBuffer command_buffer)
    : m_device(std::move(device)),
      m_desc(std::move(desc)),
      m_pool(std::move(pool)),
      m_command_buffer(std::move(command_buffer)) {
}

Result<void> VulkanCommandBuffer::begin() {
    if (m_state == RecordingState::Encoding) {
        return Error{ErrorCode::InvalidArgument, "ICommandBuffer::begin: an encoder is still open"};
    }
    if (m_state == RecordingState::Pending) {
        if (m_pending_timeline->getValue() < m_pending_value) {
            return Error{ErrorCode::InvalidArgument, "ICommandBuffer::begin: the last submission is still executing"};
        }
        m_pending_timeline.reset();
    }
    // The previous submission finished, so its resources can go.
    m_retained_resources.clear();
    if (Result<void> reset = check(m_pool.reset({}), "vkResetCommandPool"); !reset) {
        return reset;
    }
    if (Result<void> begun = check(m_command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit}),
                                   "vkBeginCommandBuffer");
        !begun) {
        return begun;
    }
    m_state = RecordingState::Recording;
    return {};
}

void VulkanCommandBuffer::barrier(BarrierBatch batch) {
    if (m_state != RecordingState::Recording) {
        return;
    }
    vk::PipelineStageFlags src_stages;
    vk::PipelineStageFlags dst_stages;

    std::vector<vk::BufferMemoryBarrier> buffer_barriers;
    for (BufferBarrier& b : batch.buffers) {
        if (!b.buffer) {
            continue;
        }
        const StageAccess before = toStageAccess(b.before);
        const StageAccess after = toStageAccess(b.after);
        src_stages |= before.stages;
        dst_stages |= after.stages;
        buffer_barriers.push_back({
            .srcAccessMask = before.access,
            .dstAccessMask = after.access,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = static_cast<const VulkanBuffer&>(*b.buffer).handle(),
            .offset = b.offset,
            .size = b.size == kWholeSize ? vk::WholeSize : b.size,
        });
        retain(std::move(b.buffer));
    }

    std::vector<vk::ImageMemoryBarrier> image_barriers;
    for (TextureBarrier& t : batch.textures) {
        if (!t.texture) {
            continue;
        }
        const StageAccess before = toStageAccess(t.before);
        const StageAccess after = toStageAccess(t.after);
        src_stages |= before.stages;
        dst_stages |= after.stages;

        const auto& texture = static_cast<const VulkanTexture&>(*t.texture);
        image_barriers.push_back({
            .srcAccessMask = before.access,
            .dstAccessMask = after.access,
            .oldLayout = toImageLayout(t.before),
            .newLayout = toImageLayout(t.after),
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = texture.handle(),
            // Barriers cover every aspect, so depth and stencil of one texture always share a layout.
            // kRemaining equals VK_REMAINING_MIP_LEVELS / VK_REMAINING_ARRAY_LAYERS.
            .subresourceRange =
                {
                    .aspectMask = aspectsOf(texture.getDesc().format),
                    .baseMipLevel = t.range.base_mip_level,
                    .levelCount = t.range.mip_level_count,
                    .baseArrayLayer = t.range.base_array_layer,
                    .layerCount = t.range.array_layer_count,
                },
        });
        retain(std::move(t.texture));
    }

    if (buffer_barriers.empty() && image_barriers.empty()) {
        return;
    }
    m_command_buffer.pipelineBarrier(src_stages, dst_stages, {}, nullptr, buffer_barriers, image_barriers);
}

RenderEncoderRef VulkanCommandBuffer::beginRender(RenderBeginDesc desc) {
    if (m_state != RecordingState::Recording || !desc.render_pass || !desc.target) {
        return nullptr;
    }
    const RenderPassDesc pass_desc = desc.render_pass->getDesc();
    const Extent2D extent = desc.target->getDesc().extent;

    // Vulkan indexes clear values by attachment; RenderBeginDesc lists colours only for clearing attachments.
    std::vector<vk::ClearValue> clear_values;
    std::size_t next_color = 0;
    for (const AttachmentDesc& attachment : pass_desc.color_attachments) {
        vk::ClearValue value{};
        if (attachment.load_op == LoadOp::Clear && next_color < desc.clear_colors.size()) {
            const std::array<float, 4>& color = desc.clear_colors[next_color++];
            value.color.float32 = color;
        }
        clear_values.push_back(value);
    }
    if (pass_desc.depth_stencil_attachment) {
        vk::ClearValue value{};
        value.depthStencil = vk::ClearDepthStencilValue{
            .depth = desc.clear_depth_stencil.depth,
            .stencil = desc.clear_depth_stencil.stencil,
        };
        clear_values.push_back(value);
    }

    const vk::Rect2D area{.offset = {0, 0}, .extent = {extent.width, extent.height}};
    m_command_buffer.beginRenderPass(
        {
            .renderPass = static_cast<const VulkanRenderPass&>(*desc.render_pass).handle(),
            .framebuffer = static_cast<const VulkanRenderTarget&>(*desc.target).handle(),
            .renderArea = area,
            .clearValueCount = static_cast<std::uint32_t>(clear_values.size()),
            .pClearValues = clear_values.data(),
        },
        vk::SubpassContents::eInline);

    // IRenderEncoder promises a viewport and scissor covering the whole target; pipelines make them dynamic.
    m_command_buffer.setViewport(0, vk::Viewport{
                                        .width = static_cast<float>(extent.width),
                                        .height = static_cast<float>(extent.height),
                                        .maxDepth = 1.0f,
                                    });
    m_command_buffer.setScissor(0, area);
    // Every pipeline makes the stencil reference dynamic too; IRenderEncoder promises it starts at 0.
    m_command_buffer.setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, 0);

    retain(std::move(desc.render_pass));
    retain(std::move(desc.target));
    m_state = RecordingState::Encoding;
    return std::make_unique<VulkanRenderEncoder>(shared_from_this(), extent);
}

ComputeEncoderRef VulkanCommandBuffer::beginCompute() {
    if (m_state != RecordingState::Recording) {
        return nullptr;
    }
    m_state = RecordingState::Encoding;
    return std::make_unique<VulkanComputeEncoder>(shared_from_this());
}

CopyEncoderRef VulkanCommandBuffer::beginCopy() {
    if (m_state != RecordingState::Recording) {
        return nullptr;
    }
    m_state = RecordingState::Encoding;
    return std::make_unique<VulkanCopyEncoder>(shared_from_this());
}

Result<void> VulkanCommandBuffer::end() {
    if (m_state != RecordingState::Recording) {
        return Error{ErrorCode::InvalidArgument, "ICommandBuffer::end: not recording, or an encoder is still open"};
    }
    // A fence wait alone does not make device writes visible to the host; this barrier does.
    const vk::MemoryBarrier to_host{
        .srcAccessMask = vk::AccessFlagBits::eMemoryWrite,
        .dstAccessMask = vk::AccessFlagBits::eHostRead,
    };
    m_command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eHost, {},
                                     to_host, nullptr, nullptr);

    if (Result<void> ended = check(m_command_buffer.end(), "vkEndCommandBuffer"); !ended) {
        return ended;
    }
    m_state = RecordingState::Executable;
    return {};
}

void VulkanCommandBuffer::markSubmitted(std::shared_ptr<VulkanSemaphore> timeline, std::uint64_t value) {
    m_state = RecordingState::Pending;
    m_pending_timeline = std::move(timeline);
    m_pending_value = value;
}

void VulkanCommandBuffer::retire(std::uint64_t value) {
    if (m_state == RecordingState::Pending && m_pending_value == value) {
        m_retained_resources.clear();
    }
}

void VulkanCommandBuffer::retain(std::shared_ptr<const void> resource) {
    m_retained_resources.push_back(std::move(resource));
}

void VulkanCommandBuffer::endEncoder() {
    if (m_state == RecordingState::Encoding) {
        m_state = RecordingState::Recording;
    }
}

} // namespace rhi::vulkan
