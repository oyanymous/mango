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

/**
 * @brief The pipeline stages and memory accesses a resource in a given state is used with.
 */
struct StageAccess {
    VkPipelineStageFlags stages;
    VkAccessFlags access;
};

/**
 * @brief Maps a ResourceState to the stages and accesses that use it, for barriers.
 */
StageAccess toStageAccess(ResourceState state) {
    constexpr VkPipelineStageFlags kShaders = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    constexpr VkPipelineStageFlags kFragmentTests =
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    switch (state) {
    case ResourceState::Undefined:
        return {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0};
    case ResourceState::CopySrc:
        return {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT};
    case ResourceState::CopyDst:
        return {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT};
    case ResourceState::VertexBuffer:
        return {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT};
    case ResourceState::IndexBuffer:
        return {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT};
    case ResourceState::UniformBuffer:
        return {kShaders, VK_ACCESS_UNIFORM_READ_BIT};
    case ResourceState::ShaderRead:
        return {kShaders, VK_ACCESS_SHADER_READ_BIT};
    case ResourceState::ShaderWrite:
        return {kShaders, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT};
    case ResourceState::RenderTarget:
        return {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
    case ResourceState::DepthStencilWrite:
        return {kFragmentTests,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
    case ResourceState::DepthStencilRead:
        return {kFragmentTests | kShaders, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT};
    case ResourceState::Present:
        return {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0};
    }
    return {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT};
}

/**
 * @brief The image layout a texture in a given state is in. States only buffers use map to GENERAL.
 */
VkImageLayout toImageLayout(ResourceState state) {
    switch (state) {
    case ResourceState::Undefined:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case ResourceState::CopySrc:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case ResourceState::CopyDst:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ResourceState::ShaderRead:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ResourceState::RenderTarget:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case ResourceState::DepthStencilWrite:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case ResourceState::DepthStencilRead:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case ResourceState::Present:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    case ResourceState::ShaderWrite:
    case ResourceState::VertexBuffer:
    case ResourceState::IndexBuffer:
    case ResourceState::UniformBuffer:
        return VK_IMAGE_LAYOUT_GENERAL;
    }
    return VK_IMAGE_LAYOUT_GENERAL;
}

} // namespace

Result<CommandBufferRef> VulkanCommandBuffer::create(std::shared_ptr<VulkanDeviceHandle> device, CommandBufferDesc desc,
                                                     std::uint32_t family_index) {
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = family_index;
    VkCommandPool pool = VK_NULL_HANDLE;
    if (VkResult result = vkCreateCommandPool(device->m_device, &pool_info, nullptr, &pool); result != VK_SUCCESS) {
        return toError(result, "vkCreateCommandPool");
    }

    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    if (VkResult result = vkAllocateCommandBuffers(device->m_device, &allocate_info, &command_buffer);
        result != VK_SUCCESS) {
        vkDestroyCommandPool(device->m_device, pool, nullptr);
        return toError(result, "vkAllocateCommandBuffers");
    }
    return CommandBufferRef(
        std::make_shared<VulkanCommandBuffer>(std::move(device), std::move(desc), pool, command_buffer));
}

VulkanCommandBuffer::VulkanCommandBuffer(std::shared_ptr<VulkanDeviceHandle> device, CommandBufferDesc desc,
                                         VkCommandPool pool, VkCommandBuffer command_buffer)
    : m_desc(std::move(desc)), m_pool(pool), m_command_buffer(command_buffer), m_device(std::move(device)) {
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
    vkDestroyCommandPool(m_device->m_device, m_pool, nullptr);
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
    if (VkResult result = vkResetCommandPool(m_device->m_device, m_pool, 0); result != VK_SUCCESS) {
        return toError(result, "vkResetCommandPool");
    }
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (VkResult result = vkBeginCommandBuffer(m_command_buffer, &begin_info); result != VK_SUCCESS) {
        return toError(result, "vkBeginCommandBuffer");
    }
    m_state = RecordingState::Recording;
    return {};
}

void VulkanCommandBuffer::barrier(BarrierBatch batch) {
    if (m_state != RecordingState::Recording) {
        return;
    }
    VkPipelineStageFlags src_stages = 0;
    VkPipelineStageFlags dst_stages = 0;

    std::vector<VkBufferMemoryBarrier> buffer_barriers;
    for (BufferBarrier& b : batch.buffers) {
        if (!b.buffer) {
            continue;
        }
        const StageAccess before = toStageAccess(b.before);
        const StageAccess after = toStageAccess(b.after);
        src_stages |= before.stages;
        dst_stages |= after.stages;

        VkBufferMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = before.access;
        barrier.dstAccessMask = after.access;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = static_cast<const VulkanBuffer&>(*b.buffer).handle();
        barrier.offset = b.offset;
        barrier.size = b.size == kWholeSize ? VK_WHOLE_SIZE : b.size;
        buffer_barriers.push_back(barrier);
        retain(std::move(b.buffer));
    }

    std::vector<VkImageMemoryBarrier> image_barriers;
    for (TextureBarrier& t : batch.textures) {
        if (!t.texture) {
            continue;
        }
        const StageAccess before = toStageAccess(t.before);
        const StageAccess after = toStageAccess(t.after);
        src_stages |= before.stages;
        dst_stages |= after.stages;

        const auto& texture = static_cast<const VulkanTexture&>(*t.texture);
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = before.access;
        barrier.dstAccessMask = after.access;
        barrier.oldLayout = toImageLayout(t.before);
        barrier.newLayout = toImageLayout(t.after);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.handle();
        // Barriers cover every aspect, so depth and stencil of one texture always share a layout.
        // kRemaining equals VK_REMAINING_MIP_LEVELS / VK_REMAINING_ARRAY_LAYERS.
        barrier.subresourceRange = {aspectsOf(texture.getDesc().format), t.range.base_mip_level,
                                    t.range.mip_level_count, t.range.base_array_layer, t.range.array_layer_count};
        image_barriers.push_back(barrier);
        retain(std::move(t.texture));
    }

    if (buffer_barriers.empty() && image_barriers.empty()) {
        return;
    }
    vkCmdPipelineBarrier(m_command_buffer, src_stages, dst_stages, 0, 0, nullptr,
                         static_cast<std::uint32_t>(buffer_barriers.size()), buffer_barriers.data(),
                         static_cast<std::uint32_t>(image_barriers.size()), image_barriers.data());
}

RenderEncoderRef VulkanCommandBuffer::beginRender(RenderBeginDesc desc) {
    if (m_state != RecordingState::Recording || !desc.render_pass || !desc.target) {
        return nullptr;
    }
    const RenderPassDesc pass_desc = desc.render_pass->getDesc();
    const Extent2D extent = desc.target->getDesc().extent;

    // Vulkan indexes clear values by attachment; RenderBeginDesc lists colours only for clearing attachments.
    std::vector<VkClearValue> clear_values;
    std::size_t next_color = 0;
    for (const AttachmentDesc& attachment : pass_desc.color_attachments) {
        VkClearValue value{};
        if (attachment.load_op == LoadOp::Clear && next_color < desc.clear_colors.size()) {
            const std::array<float, 4>& color = desc.clear_colors[next_color++];
            value.color = {{color[0], color[1], color[2], color[3]}};
        }
        clear_values.push_back(value);
    }
    if (pass_desc.depth_stencil_attachment) {
        VkClearValue value{};
        value.depthStencil = {desc.clear_depth_stencil.depth, desc.clear_depth_stencil.stencil};
        clear_values.push_back(value);
    }

    VkRenderPassBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = static_cast<const VulkanRenderPass&>(*desc.render_pass).handle();
    begin_info.framebuffer = static_cast<const VulkanRenderTarget&>(*desc.target).handle();
    begin_info.renderArea = {{0, 0}, {extent.width, extent.height}};
    begin_info.clearValueCount = static_cast<std::uint32_t>(clear_values.size());
    begin_info.pClearValues = clear_values.data();
    vkCmdBeginRenderPass(m_command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // IRenderEncoder promises a viewport and scissor covering the whole target; pipelines make them dynamic.
    const VkViewport viewport{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height),
                              0.0f, 1.0f};
    vkCmdSetViewport(m_command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(m_command_buffer, 0, 1, &begin_info.renderArea);

    retain(std::move(desc.render_pass));
    retain(std::move(desc.target));
    m_state = RecordingState::Encoding;
    return std::make_unique<VulkanRenderEncoder>(shared_from_this());
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
    VkMemoryBarrier to_host{};
    to_host.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    to_host.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    to_host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(m_command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1,
                         &to_host, 0, nullptr, 0, nullptr);

    if (VkResult result = vkEndCommandBuffer(m_command_buffer); result != VK_SUCCESS) {
        return toError(result, "vkEndCommandBuffer");
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
