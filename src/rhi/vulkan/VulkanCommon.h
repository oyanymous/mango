#pragma once

// Vulkan-Hpp first: volk.h then reuses its vulkan.h (both with VK_NO_PROTOTYPES).
#include "VulkanHpp.h"
#include <volk.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "RHITypes.h"
#include "Result.h"

/**
 * The Vulkan implementation of the RHI.
 *
 * Everything here is private to the RHI library; applications use the interfaces in namespace rhi.
 * Vulkan functions are loaded at run time by volk, so nothing links against the Vulkan loader.
 */
namespace rhi::vulkan {

/// The Vulkan version the backend is written against; adapters below it are not listed.
inline constexpr std::uint32_t kApiVersion = VK_API_VERSION_1_2;

/// Returns the enumerator name of a VkResult, e.g. `"VK_ERROR_DEVICE_LOST"`.
std::string_view toString(VkResult result);

/**
 * Maps a failed VkResult to an RHI Error.
 *
 * @param what The call that failed, e.g. `"vkCreateInstance"`; included in the message.
 * @return An Error whose code best matches @p result.
 */
Error toError(VkResult result, std::string_view what);

/// Maps a failed vk::Result to an RHI Error, like the VkResult overload.
inline Error toError(vk::Result result, std::string_view what) {
    return toError(static_cast<VkResult>(result), what);
}

/// Turns a Vulkan-Hpp result into an RHI result; on failure the error message names @p what.
template <typename T>
Result<T> check(vk::ResultValue<T>&& result, std::string_view what) {
    if (result.result != vk::Result::eSuccess) {
        return toError(result.result, what);
    }
    return std::move(result.value);
}

inline Result<void> check(vk::Result result, std::string_view what) {
    if (result != vk::Result::eSuccess) {
        return toError(result, what);
    }
    return {};
}

/// Maps RHI pipeline stages to Vulkan stage flags. PipelineStage::None maps to TOP_OF_PIPE (nothing waits).
VkPipelineStageFlags toVkPipelineStages(PipelineStage stages);

/// Maps an RHI format to its VkFormat. Format::Undefined maps to VK_FORMAT_UNDEFINED.
VkFormat toVkFormat(Format format);

/// The image aspects a format has: COLOR, or DEPTH and/or STENCIL for depth/stencil formats.
VkImageAspectFlags aspectsOf(Format format);

/// Maps a CompareOp to its VkCompareOp.
VkCompareOp toVkCompareOp(CompareOp op);

} // namespace rhi::vulkan
