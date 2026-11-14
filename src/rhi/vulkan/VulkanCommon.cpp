/// @file
/// VkResult naming and mapping to RHI errors.

#include "VulkanCommon.h"

namespace rhi::vulkan {

std::string_view toString(VkResult result) {
    switch (result) {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    default:
        return "VkResult(other)";
    }
}

Error toError(VkResult result, std::string_view what) {
    ErrorCode code = ErrorCode::Unknown;
    switch (result) {
    case VK_TIMEOUT:
        code = ErrorCode::Timeout;
        break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        code = ErrorCode::OutOfHostMemory;
        break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        code = ErrorCode::OutOfDeviceMemory;
        break;
    case VK_ERROR_INITIALIZATION_FAILED:
        code = ErrorCode::InitializationFailed;
        break;
    case VK_ERROR_DEVICE_LOST:
        code = ErrorCode::DeviceLost;
        break;
    case VK_ERROR_SURFACE_LOST_KHR:
        code = ErrorCode::SurfaceLost;
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        code = ErrorCode::OutOfDate;
        break;
    case VK_ERROR_LAYER_NOT_PRESENT:
    case VK_ERROR_EXTENSION_NOT_PRESENT:
    case VK_ERROR_FEATURE_NOT_PRESENT:
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        code = ErrorCode::Unsupported;
        break;
    default:
        break;
    }
    std::string message(what);
    message += " failed: ";
    message += toString(result);
    if (toString(result) == "VkResult(other)") {
        message += " (" + std::to_string(static_cast<int>(result)) + ")";
    }
    return Error{code, std::move(message)};
}

VkPipelineStageFlags toVkPipelineStages(PipelineStage stages) {
    if (hasFlag(stages, PipelineStage::AllCommands)) {
        return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
    VkPipelineStageFlags flags = 0;
    if (hasFlag(stages, PipelineStage::VertexInput)) {
        flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    }
    if (hasFlag(stages, PipelineStage::VertexShader)) {
        flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    }
    if (hasFlag(stages, PipelineStage::FragmentShader)) {
        flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    if (hasFlag(stages, PipelineStage::ComputeShader)) {
        flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    if (hasFlag(stages, PipelineStage::RenderTargetOutput)) {
        flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    if (hasFlag(stages, PipelineStage::DepthStencil)) {
        flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }
    if (hasFlag(stages, PipelineStage::Copy)) {
        flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    return flags != 0 ? flags : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
}

} // namespace rhi::vulkan
