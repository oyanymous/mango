#pragma once

#include "VulkanHpp.h"

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
 * Vulkan objects are vk::raii wrappers that call through their own dispatch tables, loaded at run time from the
 * driver's Vulkan loader, so nothing links against it.
 */
namespace rhi::vulkan {

/// The Vulkan version the backend is written against; adapters below it are not listed.
inline constexpr std::uint32_t kApiVersion = VK_API_VERSION_1_2;

/// Returns the enumerator name of a result, e.g. `"VK_ERROR_DEVICE_LOST"`.
std::string_view toString(vk::Result result);

/**
 * Maps a failed result to an RHI Error.
 *
 * @param what The call that failed, e.g. `"vkCreateInstance"`; included in the message.
 * @return An Error whose code best matches @p result.
 */
Error toError(vk::Result result, std::string_view what);

/// For the VMA functions, which return VkResult.
inline Error toError(VkResult result, std::string_view what) {
    return toError(static_cast<vk::Result>(result), what);
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
vk::PipelineStageFlags toVkPipelineStages(PipelineStage stages);

/// Format::Undefined maps to vk::Format::eUndefined.
vk::Format toVkFormat(Format format);

/// The image aspects a format has: COLOR, or DEPTH and/or STENCIL for depth/stencil formats.
vk::ImageAspectFlags aspectsOf(Format format);

vk::CompareOp toVkCompareOp(CompareOp op);

} // namespace rhi::vulkan
