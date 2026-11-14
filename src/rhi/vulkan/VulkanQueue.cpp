/**
 * @file
 * @brief VulkanQueue.
 */

#include "VulkanQueue.h"

#include <vector>

#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanFence.h"
#include "VulkanSemaphore.h"
#include "VulkanSwapchain.h"

namespace rhi::vulkan {

Result<std::shared_ptr<VulkanQueue>> VulkanQueue::create(std::shared_ptr<VulkanDeviceHandle> device, QueueType type,
                                                         std::uint32_t family_index) {
    Result<SemaphoreRef> timeline = VulkanSemaphore::create(device, {.kind = SemaphoreKind::Timeline});
    if (!timeline) {
        return timeline.error();
    }
    return std::make_shared<VulkanQueue>(std::move(device), type, family_index,
                                         std::static_pointer_cast<VulkanSemaphore>(std::move(timeline).value()));
}

VulkanQueue::VulkanQueue(std::shared_ptr<VulkanDeviceHandle> device, QueueType type, std::uint32_t family_index,
                         std::shared_ptr<VulkanSemaphore> timeline)
    : m_queue_type(type), m_family_index(family_index), m_timeline(std::move(timeline)), m_device(std::move(device)) {
    vkGetDeviceQueue(m_device->m_device, family_index, 0, &m_queue);
}

VulkanQueue::~VulkanQueue() {
    // Also covers presents, whose semaphore waits no fence or timeline can observe.
    vkQueueWaitIdle(m_queue);
    for (InFlight& submission : m_in_flight) {
        for (const CommandBufferRef& command_buffer : submission.command_buffers) {
            static_cast<VulkanCommandBuffer&>(*command_buffer).retire(submission.value);
        }
    }
}

void VulkanQueue::retireFinished() {
    const std::uint64_t reached = m_timeline->getValue();
    while (!m_in_flight.empty() && m_in_flight.front().value <= reached) {
        InFlight& submission = m_in_flight.front();
        for (const CommandBufferRef& command_buffer : submission.command_buffers) {
            static_cast<VulkanCommandBuffer&>(*command_buffer).retire(submission.value);
        }
        m_in_flight.pop_front();
    }
}

Result<void> VulkanQueue::submit(SubmitDesc desc) {
    retireFinished();

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
    // The queue's own timeline marks when this submission, and so everything it holds, is finished.
    const std::uint64_t value = m_last_value + 1;
    signal_semaphores.push_back(m_timeline->handle());
    signal_values.push_back(value);

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
    m_last_value = value;

    InFlight submission{.value = value, .command_buffers = std::move(desc.command_buffers), .fence = desc.fence};
    for (const CommandBufferRef& command_buffer : submission.command_buffers) {
        static_cast<VulkanCommandBuffer&>(*command_buffer).markSubmitted(m_timeline, value);
    }
    for (SemaphoreWait& wait : desc.waits) {
        submission.semaphores.push_back(std::move(wait.semaphore));
    }
    for (SemaphoreSignal& signal : desc.signals) {
        submission.semaphores.push_back(std::move(signal.semaphore));
    }
    m_in_flight.push_back(std::move(submission));
    return {};
}

Result<void> VulkanQueue::present(PresentDesc desc) {
    retireFinished();
    if (!desc.swapchain) {
        return Error{ErrorCode::InvalidArgument, "VulkanQueue::present: swapchain is null"};
    }
    const VkSwapchainKHR swapchain = static_cast<const VulkanSwapchain&>(*desc.swapchain).handle();
    if (swapchain == VK_NULL_HANDLE) {
        return Error{ErrorCode::OutOfDate, "VulkanQueue::present: the swapchain has no images (zero extent)"};
    }

    std::vector<VkSemaphore> wait_semaphores;
    for (const SemaphoreRef& semaphore : desc.wait_semaphores) {
        if (!semaphore) {
            return Error{ErrorCode::InvalidArgument, "VulkanQueue::present: null wait semaphore"};
        }
        wait_semaphores.push_back(static_cast<const VulkanSemaphore&>(*semaphore).handle());
    }

    VkPresentInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = static_cast<std::uint32_t>(wait_semaphores.size());
    info.pWaitSemaphores = wait_semaphores.data();
    info.swapchainCount = 1;
    info.pSwapchains = &swapchain;
    info.pImageIndices = &desc.image_index;

    const VkResult result = vkQueuePresentKHR(m_queue, &info);
    // Even a failed present may have consumed the waits; replacing the entry releases this image's last present.
    m_presented[{desc.swapchain.get(), desc.image_index}] = std::move(desc.wait_semaphores);
    // A suboptimal swapchain still presented, but should be resized like an out-of-date one.
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
        return Error{ErrorCode::OutOfDate, "VulkanQueue::present: the swapchain must be resized"};
    }
    if (result != VK_SUCCESS) {
        return toError(result, "vkQueuePresentKHR");
    }
    return {};
}

} // namespace rhi::vulkan
