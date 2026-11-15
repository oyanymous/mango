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
    : m_device(std::move(device)),
      m_queue_type(type),
      m_family_index(family_index),
      m_queue(m_device->m_device.getQueue(family_index, 0)),
      m_timeline(std::move(timeline)) {
}

VulkanQueue::~VulkanQueue() {
    (void)m_queue.waitIdle();
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

    std::vector<vk::CommandBuffer> command_buffers;
    for (const CommandBufferRef& command_buffer : desc.command_buffers) {
        const auto* vk = static_cast<const VulkanCommandBuffer*>(command_buffer.get());
        if (!vk || vk->state() != VulkanCommandBuffer::RecordingState::Executable) {
            return Error{ErrorCode::InvalidArgument, "VulkanQueue::submit: command buffer is null or not ended"};
        }
        command_buffers.emplace_back(vk->handle());
    }

    std::vector<vk::Semaphore> wait_semaphores;
    std::vector<vk::PipelineStageFlags> wait_stages;
    std::vector<std::uint64_t> wait_values;
    for (const SemaphoreWait& wait : desc.waits) {
        if (!wait.semaphore) {
            return Error{ErrorCode::InvalidArgument, "VulkanQueue::submit: null wait semaphore"};
        }
        wait_semaphores.emplace_back(static_cast<const VulkanSemaphore&>(*wait.semaphore).handle());
        wait_stages.emplace_back(toVkPipelineStages(wait.stage));
        wait_values.push_back(wait.value);
    }
    std::vector<vk::Semaphore> signal_semaphores;
    std::vector<std::uint64_t> signal_values;
    for (const SemaphoreSignal& signal : desc.signals) {
        if (!signal.semaphore) {
            return Error{ErrorCode::InvalidArgument, "VulkanQueue::submit: null signal semaphore"};
        }
        signal_semaphores.emplace_back(static_cast<const VulkanSemaphore&>(*signal.semaphore).handle());
        signal_values.push_back(signal.value);
    }

    const std::uint64_t value = m_last_value + 1;
    signal_semaphores.emplace_back(m_timeline->handle());
    signal_values.push_back(value);

    const vk::TimelineSemaphoreSubmitInfo timeline_info{
        .waitSemaphoreValueCount = static_cast<std::uint32_t>(wait_values.size()),
        .pWaitSemaphoreValues = wait_values.data(),
        .signalSemaphoreValueCount = static_cast<std::uint32_t>(signal_values.size()),
        .pSignalSemaphoreValues = signal_values.data(),
    };
    const vk::SubmitInfo submit_info{
        .pNext = &timeline_info,
        .waitSemaphoreCount = static_cast<std::uint32_t>(wait_semaphores.size()),
        .pWaitSemaphores = wait_semaphores.data(),
        .pWaitDstStageMask = wait_stages.data(),
        .commandBufferCount = static_cast<std::uint32_t>(command_buffers.size()),
        .pCommandBuffers = command_buffers.data(),
        .signalSemaphoreCount = static_cast<std::uint32_t>(signal_semaphores.size()),
        .pSignalSemaphores = signal_semaphores.data(),
    };

    const vk::Fence fence = desc.fence ? static_cast<const VulkanFence&>(*desc.fence).handle() : nullptr;
    if (Result<void> submitted = check(m_queue.submit(submit_info, fence), "vkQueueSubmit"); !submitted) {
        return submitted;
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
    const vk::SwapchainKHR swapchain = static_cast<const VulkanSwapchain&>(*desc.swapchain).handle();
    if (!swapchain) {
        return Error{ErrorCode::OutOfDate, "VulkanQueue::present: the swapchain has no images (zero extent)"};
    }

    std::vector<vk::Semaphore> wait_semaphores;
    for (const SemaphoreRef& semaphore : desc.wait_semaphores) {
        if (!semaphore) {
            return Error{ErrorCode::InvalidArgument, "VulkanQueue::present: null wait semaphore"};
        }
        wait_semaphores.emplace_back(static_cast<const VulkanSemaphore&>(*semaphore).handle());
    }

    const vk::PresentInfoKHR info{
        .waitSemaphoreCount = static_cast<std::uint32_t>(wait_semaphores.size()),
        .pWaitSemaphores = wait_semaphores.data(),
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &desc.image_index,
    };

    const vk::Result result = m_queue.presentKHR(info);

    m_presented[{desc.swapchain.get(), desc.image_index}] = std::move(desc.wait_semaphores);

    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
        return Error{ErrorCode::OutOfDate, "VulkanQueue::present: the swapchain must be resized"};
    }
    return check(result, "vkQueuePresentKHR");
}

} // namespace rhi::vulkan
