/// @file
/// VulkanQueue.

#include "VulkanQueue.h"

#include <vector>

#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanFence.h"
#include "VulkanSemaphore.h"

namespace rhi::vulkan {

VulkanQueue::VulkanQueue(std::shared_ptr<VulkanDeviceHandle> device, QueueType type, std::uint32_t family_index)
    : m_queue_type(type), m_family_index(family_index), m_device(std::move(device)) {
    vkGetDeviceQueue(m_device->m_device, family_index, 0, &m_queue);
}

Result<void> VulkanQueue::submit(SubmitDesc desc) {
    std::vector<VkCommandBuffer> command_buffers;
    for (const CommandBufferRef& command_buffer : desc.command_buffers) {
        const auto* vk = static_cast<const VulkanCommandBuffer*>(command_buffer.get());
        if (!vk || vk->state() != VulkanCommandBuffer::RecordingState::Executable) {
            return Error{ErrorCode::InvalidArgument, "VulkanQueue::submit: command buffer is null or not ended"};
        }
        command_buffers.push_back(vk->handle());
    }

    // Timeline values are per semaphore; Vulkan ignores the entries of binary semaphores.
    std::vector<VkSemaphore> wait_semaphores;
    std::vector<VkPipelineStageFlags> wait_stages;
    std::vector<std::uint64_t> wait_values;
    for (const SemaphoreWait& wait : desc.waits) {
        if (!wait.semaphore) {
            return Error{ErrorCode::InvalidArgument, "VulkanQueue::submit: null wait semaphore"};
        }
        wait_semaphores.push_back(static_cast<const VulkanSemaphore&>(*wait.semaphore).handle());
        wait_stages.push_back(toVkPipelineStages(wait.stage));
        wait_values.push_back(wait.value);
    }
    std::vector<VkSemaphore> signal_semaphores;
    std::vector<std::uint64_t> signal_values;
    for (const SemaphoreSignal& signal : desc.signals) {
        if (!signal.semaphore) {
            return Error{ErrorCode::InvalidArgument, "VulkanQueue::submit: null signal semaphore"};
        }
        signal_semaphores.push_back(static_cast<const VulkanSemaphore&>(*signal.semaphore).handle());
        signal_values.push_back(signal.value);
    }

    VkTimelineSemaphoreSubmitInfo timeline_info{};
    timeline_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timeline_info.waitSemaphoreValueCount = static_cast<std::uint32_t>(wait_values.size());
    timeline_info.pWaitSemaphoreValues = wait_values.data();
    timeline_info.signalSemaphoreValueCount = static_cast<std::uint32_t>(signal_values.size());
    timeline_info.pSignalSemaphoreValues = signal_values.data();

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = &timeline_info;
    submit_info.waitSemaphoreCount = static_cast<std::uint32_t>(wait_semaphores.size());
    submit_info.pWaitSemaphores = wait_semaphores.data();
    submit_info.pWaitDstStageMask = wait_stages.data();
    submit_info.commandBufferCount = static_cast<std::uint32_t>(command_buffers.size());
    submit_info.pCommandBuffers = command_buffers.data();
    submit_info.signalSemaphoreCount = static_cast<std::uint32_t>(signal_semaphores.size());
    submit_info.pSignalSemaphores = signal_semaphores.data();

    const VkFence fence = desc.fence ? static_cast<const VulkanFence&>(*desc.fence).handle() : VK_NULL_HANDLE;
    if (VkResult result = vkQueueSubmit(m_queue, 1, &submit_info, fence); result != VK_SUCCESS) {
        return toError(result, "vkQueueSubmit");
    }
    return {};
}

Result<void> VulkanQueue::present(PresentDesc) {
    return Error{ErrorCode::Unsupported, "VulkanQueue::present is implemented in step 9"};
}

} // namespace rhi::vulkan
