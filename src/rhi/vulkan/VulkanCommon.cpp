/**
 * @file
 * @brief vk::Result naming and mapping to RHI errors, and RHI-to-Vulkan enum mappings.
 */

#include "VulkanCommon.h"

namespace rhi::vulkan {

std::string_view toString(vk::Result result) {
    switch (result) {
    case vk::Result::eSuccess:
        return "VK_SUCCESS";
    case vk::Result::eNotReady:
        return "VK_NOT_READY";
    case vk::Result::eTimeout:
        return "VK_TIMEOUT";
    case vk::Result::eIncomplete:
        return "VK_INCOMPLETE";
    case vk::Result::eSuboptimalKHR:
        return "VK_SUBOPTIMAL_KHR";
    case vk::Result::eErrorOutOfHostMemory:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case vk::Result::eErrorOutOfDeviceMemory:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case vk::Result::eErrorInitializationFailed:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case vk::Result::eErrorDeviceLost:
        return "VK_ERROR_DEVICE_LOST";
    case vk::Result::eErrorLayerNotPresent:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case vk::Result::eErrorExtensionNotPresent:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case vk::Result::eErrorFeatureNotPresent:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case vk::Result::eErrorIncompatibleDriver:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case vk::Result::eErrorSurfaceLostKHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case vk::Result::eErrorOutOfDateKHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    default:
        return "VkResult(other)";
    }
}

Error toError(vk::Result result, std::string_view what) {
    ErrorCode code = ErrorCode::Unknown;
    switch (result) {
    case vk::Result::eTimeout:
        code = ErrorCode::Timeout;
        break;
    case vk::Result::eErrorOutOfHostMemory:
        code = ErrorCode::OutOfHostMemory;
        break;
    case vk::Result::eErrorOutOfDeviceMemory:
        code = ErrorCode::OutOfDeviceMemory;
        break;
    case vk::Result::eErrorInitializationFailed:
        code = ErrorCode::InitializationFailed;
        break;
    case vk::Result::eErrorDeviceLost:
        code = ErrorCode::DeviceLost;
        break;
    case vk::Result::eErrorSurfaceLostKHR:
        code = ErrorCode::SurfaceLost;
        break;
    case vk::Result::eErrorOutOfDateKHR:
        code = ErrorCode::OutOfDate;
        break;
    case vk::Result::eErrorLayerNotPresent:
    case vk::Result::eErrorExtensionNotPresent:
    case vk::Result::eErrorFeatureNotPresent:
    case vk::Result::eErrorIncompatibleDriver:
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

vk::PipelineStageFlags toVkPipelineStages(PipelineStage stages) {
    using Stage = vk::PipelineStageFlagBits;
    if (hasFlag(stages, PipelineStage::AllCommands)) {
        return Stage::eAllCommands;
    }
    vk::PipelineStageFlags flags;
    if (hasFlag(stages, PipelineStage::VertexInput)) {
        flags |= Stage::eVertexInput;
    }
    if (hasFlag(stages, PipelineStage::VertexShader)) {
        flags |= Stage::eVertexShader;
    }
    if (hasFlag(stages, PipelineStage::FragmentShader)) {
        flags |= Stage::eFragmentShader;
    }
    if (hasFlag(stages, PipelineStage::ComputeShader)) {
        flags |= Stage::eComputeShader;
    }
    if (hasFlag(stages, PipelineStage::RenderTargetOutput)) {
        flags |= Stage::eColorAttachmentOutput;
    }
    if (hasFlag(stages, PipelineStage::DepthStencil)) {
        flags |= Stage::eEarlyFragmentTests | Stage::eLateFragmentTests;
    }
    if (hasFlag(stages, PipelineStage::Copy)) {
        flags |= Stage::eTransfer;
    }
    return flags ? flags : Stage::eTopOfPipe;
}

vk::Format toVkFormat(Format format) {
    switch (format) {
    case Format::Undefined:
        return vk::Format::eUndefined;
    case Format::R8Unorm:
        return vk::Format::eR8Unorm;
    case Format::RG8Unorm:
        return vk::Format::eR8G8Unorm;
    case Format::RGBA8Unorm:
        return vk::Format::eR8G8B8A8Unorm;
    case Format::RGBA8Srgb:
        return vk::Format::eR8G8B8A8Srgb;
    case Format::BGRA8Unorm:
        return vk::Format::eB8G8R8A8Unorm;
    case Format::BGRA8Srgb:
        return vk::Format::eB8G8R8A8Srgb;
    case Format::R16Float:
        return vk::Format::eR16Sfloat;
    case Format::RG16Float:
        return vk::Format::eR16G16Sfloat;
    case Format::RGBA16Float:
        return vk::Format::eR16G16B16A16Sfloat;
    case Format::R32Uint:
        return vk::Format::eR32Uint;
    case Format::R32Float:
        return vk::Format::eR32Sfloat;
    case Format::RG32Float:
        return vk::Format::eR32G32Sfloat;
    case Format::RGB32Float:
        return vk::Format::eR32G32B32Sfloat;
    case Format::RGBA32Float:
        return vk::Format::eR32G32B32A32Sfloat;
    case Format::D16Unorm:
        return vk::Format::eD16Unorm;
    case Format::D24UnormS8Uint:
        return vk::Format::eD24UnormS8Uint;
    case Format::D32Float:
        return vk::Format::eD32Sfloat;
    case Format::D32FloatS8Uint:
        return vk::Format::eD32SfloatS8Uint;
    }
    return vk::Format::eUndefined;
}

vk::ImageAspectFlags aspectsOf(Format format) {
    using Aspect = vk::ImageAspectFlagBits;
    switch (format) {
    case Format::D16Unorm:
    case Format::D32Float:
        return Aspect::eDepth;
    case Format::D24UnormS8Uint:
    case Format::D32FloatS8Uint:
        return Aspect::eDepth | Aspect::eStencil;
    default:
        return Aspect::eColor;
    }
}

vk::CompareOp toVkCompareOp(CompareOp op) {
    // The enumerators are declared in Vulkan's order.
    static_assert(static_cast<int>(CompareOp::Never) == VK_COMPARE_OP_NEVER &&
                  static_cast<int>(CompareOp::GreaterOrEqual) == VK_COMPARE_OP_GREATER_OR_EQUAL &&
                  static_cast<int>(CompareOp::Always) == VK_COMPARE_OP_ALWAYS);
    return static_cast<vk::CompareOp>(static_cast<int>(op));
}

} // namespace rhi::vulkan
