#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "ICommandBuffer.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;
class VulkanSemaphore;

class VulkanCommandBuffer final : public ICommandBuffer, public std::enable_shared_from_this<VulkanCommandBuffer> {
public:
    enum class RecordingState {
        Initial,
        Recording,
        Encoding,
        Executable,
        Pending,
    };

    static Result<CommandBufferRef> create(std::shared_ptr<VulkanDeviceHandle> device, CommandBufferDesc desc,
                                           std::uint32_t family_index);

    VulkanCommandBuffer(std::shared_ptr<VulkanDeviceHandle> device, CommandBufferDesc desc, vk::raii::CommandPool pool,
                        vk::raii::CommandBuffer command_buffer);

    VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
    VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;

    Result<void> begin() override;

    void barrier(BarrierBatch batch) override;

    RenderEncoderRef beginRender(RenderBeginDesc desc) override;

    ComputeEncoderRef beginCompute() override;

    CopyEncoderRef beginCopy() override;

    Result<void> end() override;

    vk::CommandBuffer handle() const { return *m_command_buffer; }

    const vk::raii::CommandBuffer& commands() const { return m_command_buffer; }
    RecordingState state() const { return m_state; }

    void retain(std::shared_ptr<const void> resource);

    void endEncoder();

    void markSubmitted(std::shared_ptr<VulkanSemaphore> timeline, std::uint64_t value);

    void retire(std::uint64_t value);
private:
    std::shared_ptr<VulkanDeviceHandle> m_device;
    CommandBufferDesc m_desc;
    RecordingState m_state = RecordingState::Initial;
    std::vector<std::shared_ptr<const void>> m_retained_resources;
    std::shared_ptr<VulkanSemaphore> m_pending_timeline;
    std::uint64_t m_pending_value = 0;
    vk::raii::CommandPool m_pool;
    vk::raii::CommandBuffer m_command_buffer;
};

} // namespace rhi::vulkan
