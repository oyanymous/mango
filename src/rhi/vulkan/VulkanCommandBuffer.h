#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "ICommandBuffer.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;
class VulkanSemaphore;

/**
 * The Vulkan ICommandBuffer. Created by VulkanDevice::createCommandBuffer.
 *
 * Each command buffer owns its pool, so begin() can reset the pool and buffers can be recorded on different
 * threads without sharing a pool.
 */
class VulkanCommandBuffer final : public ICommandBuffer, public std::enable_shared_from_this<VulkanCommandBuffer> {
public:
    /// Where recording stands; calls in the wrong state are ignored (or return InvalidArgument).
    enum class RecordingState {
        Initial,    /**< Created or never begun. */
        Recording,  /**< Between begin() and end(), no encoder open. */
        Encoding,   /**< An encoder is open. */
        Executable, /**< end() succeeded; can be submitted. */
        Pending,    /**< Submitted; begin() fails until the GPU finished it. */
    };

    /**
     * Creates the pool and the command buffer.
     *
     * @param family_index Queue family of that type's queue.
     */
    static Result<CommandBufferRef> create(std::shared_ptr<VulkanDeviceHandle> device, CommandBufferDesc desc,
                                           std::uint32_t family_index);

    /**
     * Takes ownership of a created pool and command buffer. Use create().
     *
     * @param command_buffer The command buffer, allocated from @p pool.
     */
    VulkanCommandBuffer(std::shared_ptr<VulkanDeviceHandle> device, CommandBufferDesc desc, vk::raii::CommandPool pool,
                        vk::raii::CommandBuffer command_buffer);

    VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
    VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;

    Result<void> begin() override;
    /// Records buffer and texture barriers in one pipeline barrier; image layouts follow from the states.
    void barrier(BarrierBatch batch) override;
    /**
     * Begins the render pass, sets a viewport and scissor covering the target, and opens a
     * VulkanRenderEncoder.
     *
     * @return The encoder, or null unless recording with no encoder open and both pass and target are set.
     */
    RenderEncoderRef beginRender(RenderBeginDesc desc) override;
    /**
     * Begins compute recording.
     *
     * @return The encoder, or null unless recording with no encoder open.
     */
    ComputeEncoderRef beginCompute() override;
    /// Opens a VulkanCopyEncoder; returns null unless recording with no encoder open.
    CopyEncoderRef beginCopy() override;
    /// Makes all GPU writes visible to the host, so mapped GpuToCpu buffers read correctly after the submission's
    /// fence, then ends recording.
    Result<void> end() override;

    vk::CommandBuffer handle() const { return *m_command_buffer; }
    /// What encoders record into.
    const vk::raii::CommandBuffer& commands() const { return m_command_buffer; }
    RecordingState state() const { return m_state; }
    /**
     * Keeps a resource alive until the next begin() (or the command buffer's destruction).
     *
     * @param resource Any RHI object recorded into the command buffer.
     */
    void retain(std::shared_ptr<const void> resource);
    /// Called by an encoder's end(): recording continues on the command buffer.
    void endEncoder();
    /**
     * Called by VulkanQueue::submit: the command buffer runs until @p timeline reaches @p value.
     *
     * @param timeline The queue's timeline semaphore; kept alive by the command buffer.
     * @param value The value the submission signals on it.
     */
    void markSubmitted(std::shared_ptr<VulkanSemaphore> timeline, std::uint64_t value);
    /// Called by VulkanQueue once the submission that signals @p value finished: releases the recorded resources,
    /// unless the command buffer has been begun again since.
    void retire(std::uint64_t value);
private:
    std::shared_ptr<VulkanDeviceHandle> m_device; // first: destroyed after the pool
    CommandBufferDesc m_desc;
    RecordingState m_state = RecordingState::Initial;
    std::vector<std::shared_ptr<const void>> m_retained_resources;
    std::shared_ptr<VulkanSemaphore> m_pending_timeline; // set while Pending
    std::uint64_t m_pending_value = 0;
    vk::raii::CommandPool m_pool;
    vk::raii::CommandBuffer m_command_buffer; // after the pool: freed into it
};

} // namespace rhi::vulkan
