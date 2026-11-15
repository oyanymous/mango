#pragma once

#include "VulkanHpp.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "RHITypes.h"
#include "Result.h"

namespace rhi::vulkan {

inline constexpr std::uint32_t kApiVersion = VK_API_VERSION_1_2;

std::string_view toString(vk::Result result);

Error toError(vk::Result result, std::string_view what);

inline Error toError(VkResult result, std::string_view what) {
    return toError(static_cast<vk::Result>(result), what);
}

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

vk::PipelineStageFlags toVkPipelineStages(PipelineStage stages);

vk::Format toVkFormat(Format format);

vk::ImageAspectFlags aspectsOf(Format format);

vk::CompareOp toVkCompareOp(CompareOp op);

} // namespace rhi::vulkan
