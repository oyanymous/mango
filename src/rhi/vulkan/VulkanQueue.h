#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "IQueue.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;
class VulkanSemaphore;

class VulkanQueue final : public IQueue {
public:
    static Result<std::shared_ptr<VulkanQueue>> create(std::shared_ptr<VulkanDeviceHandle> device, QueueType type,
                                                       std::uint32_t family_index);

    VulkanQueue(std::shared_ptr<VulkanDeviceHandle> device, QueueType type, std::uint32_t family_index,
                std::shared_ptr<VulkanSemaphore> timeline);

    ~VulkanQueue() override;

    VulkanQueue(const VulkanQueue&) = delete;
    VulkanQueue& operator=(const VulkanQueue&) = delete;

    Result<void> submit(SubmitDesc desc) override;

    Result<void> present(PresentDesc desc) override;

    QueueType type() const { return m_queue_type; }

    std::uint32_t familyIndex() const { return m_family_index; }
private:
    struct InFlight {
        std::uint64_t value = 0;
        std::vector<CommandBufferRef> command_buffers;
        std::vector<SemaphoreRef> semaphores;
        FenceRef fence;
    };

    void retireFinished();

    std::shared_ptr<VulkanDeviceHandle> m_device;
    QueueType m_queue_type;
    std::uint32_t m_family_index;
    vk::raii::Queue m_queue;
    std::shared_ptr<VulkanSemaphore> m_timeline;
    std::uint64_t m_last_value = 0;
    std::deque<InFlight> m_in_flight;

    std::map<std::pair<const void*, std::uint32_t>, std::vector<SemaphoreRef>> m_presented;
};

} // namespace rhi::vulkan
