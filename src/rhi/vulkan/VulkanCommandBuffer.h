#pragma once

/**
 * @file
 * @brief rhi::vulkan::VulkanCommandBuffer: a VkCommandBuffer with its own VkCommandPool.
 */

#include <cstdint>
#include <memory>
#include <vector>

#include "ICommandBuffer.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanDeviceHandle;

/**
 * @brief The Vulkan ICommandBuffer. Created by VulkanDevice::createCommandBuffer.
 *
 * @details
 * Each command buffer owns its pool, so begin() can reset the pool and buffers can be recorded on different
 * threads without sharing a pool.
 */
class VulkanCommandBuffer final : public ICommandBuffer, public std::enable_shared_from_this<VulkanCommandBuffer> {
public:
    /**
     * @brief Where recording stands; calls in the wrong state are ignored (or return InvalidArgument).
     */
    enum class RecordingState {
        Initial,    /**< Created or never begun. */
        Recording,  /**< Between begin() and end(), no encoder open. */
        Encoding,   /**< An encoder is open. */
        Executable, /**< end() succeeded; can be submitted. */
    };

    /**
     * @brief Creates the pool and the command buffer.
     *
     * @param device Owner of the VkDevice; kept alive by the command buffer.
     * @param desc Target queue type.
     * @param family_index Queue family of that type's queue.
     */
    static Result<CommandBufferRef> create(std::shared_ptr<VulkanDeviceHandle> device, CommandBufferDesc desc,
                                           std::uint32_t family_index);

    /**
     * @brief Takes ownership of a created pool and command buffer. Use create().
     *
     * @param device Owner of the VkDevice.
     * @param desc The descriptor it was created with.
     * @param pool The pool.
     * @param command_buffer The command buffer, allocated from @p pool.
     */
    VulkanCommandBuffer(std::shared_ptr<VulkanDeviceHandle> device, CommandBufferDesc desc, VkCommandPool pool,
                        VkCommandBuffer command_buffer);
    /**
     * @brief Destroys the pool, which frees the command buffer.
     */
    ~VulkanCommandBuffer() override;

    VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
    VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;

    Result<void> begin() override;
    /**
     * @brief Records buffer and texture barriers in one vkCmdPipelineBarrier; image layouts follow from the states.
     */
    void barrier(BarrierBatch batch) override;
    /**
     * @brief Not implemented yet (step 8); returns null.
     */
    RenderEncoderRef beginRender(RenderBeginDesc desc) override;
    /**
     * @brief Not implemented yet (step 12); returns null.
     */
    ComputeEncoderRef beginCompute() override;
    /**
     * @brief Opens a VulkanCopyEncoder; returns null unless recording with no encoder open.
     */
    CopyEncoderRef beginCopy() override;
    /**
     * @brief Makes all GPU writes visible to the host, so mapped GpuToCpu buffers read correctly after the
     * submission's fence, then ends recording.
     */
    Result<void> end() override;

    /**
     * @brief The VkCommandBuffer, for submission and encoders.
     */
    VkCommandBuffer handle() const { return m_command_buffer; }
    /**
     * @brief Recording state.
     */
    RecordingState state() const { return m_state; }
    /**
     * @brief Keeps a resource alive until the next begin() (or the command buffer's destruction).
     *
     * @param resource Any RHI object recorded into the command buffer.
     */
    void retain(std::shared_ptr<const void> resource);
    /**
     * @brief Called by an encoder's end(): recording continues on the command buffer.
     */
    void endEncoder();
private:
    CommandBufferDesc m_desc;
    RecordingState m_state = RecordingState::Initial;
    std::vector<std::shared_ptr<const void>> m_retained_resources;
    VkCommandPool m_pool = VK_NULL_HANDLE;
    VkCommandBuffer m_command_buffer = VK_NULL_HANDLE;
    std::shared_ptr<VulkanDeviceHandle> m_device;
};

} // namespace rhi::vulkan
