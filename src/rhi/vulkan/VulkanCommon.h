#pragma once

/**
 * @file
 * @brief Shared helpers for the Vulkan backend. Never include from a public RHI header.
 */

#include <volk.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "RHITypes.h"
#include "Result.h"

/**
 * @brief The Vulkan implementation of the RHI.
 *
 * @details
 * Everything here is private to the RHI library; applications use the interfaces in namespace rhi.
 * Vulkan functions are loaded at run time by volk, so nothing links against the Vulkan loader.
 */
namespace rhi::vulkan {

/**
 * @brief The Vulkan version the backend is written against; adapters below it are not listed.
 */
inline constexpr std::uint32_t kApiVersion = VK_API_VERSION_1_2;

/**
 * @brief Returns the enumerator name of a VkResult, e.g. `"VK_ERROR_DEVICE_LOST"`.
 *
 * @param result Result to name.
 */
std::string_view toString(VkResult result);

/**
 * @brief Maps a failed VkResult to an RHI Error.
 *
 * @param result The failure.
 * @param what The call that failed, e.g. `"vkCreateInstance"`; included in the message.
 * @return An Error whose code best matches @p result.
 */
Error toError(VkResult result, std::string_view what);

/**
 * @brief Maps RHI pipeline stages to Vulkan stage flags. PipelineStage::None maps to TOP_OF_PIPE (nothing waits).
 *
 * @param stages Stages as a bit set.
 */
VkPipelineStageFlags toVkPipelineStages(PipelineStage stages);

/**
 * @brief Maps an RHI format to its VkFormat. Format::Undefined maps to VK_FORMAT_UNDEFINED.
 *
 * @param format Format to map.
 */
VkFormat toVkFormat(Format format);

/**
 * @brief The image aspects a format has: COLOR, or DEPTH and/or STENCIL for depth/stencil formats.
 *
 * @param format Texture format.
 */
VkImageAspectFlags aspectsOf(Format format);

/**
 * @brief Maps a CompareOp to its VkCompareOp.
 *
 * @param op Comparison.
 */
VkCompareOp toVkCompareOp(CompareOp op);

} // namespace rhi::vulkan
