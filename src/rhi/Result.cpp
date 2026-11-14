/**
 * @file
 * @brief rhi::toString(ErrorCode).
 */

#include "Result.h"

namespace rhi {

std::string_view toString(ErrorCode code) {
    switch (code) {
    case ErrorCode::Unknown:
        return "Unknown";
    case ErrorCode::InvalidArgument:
        return "InvalidArgument";
    case ErrorCode::Unsupported:
        return "Unsupported";
    case ErrorCode::BackendUnavailable:
        return "BackendUnavailable";
    case ErrorCode::InitializationFailed:
        return "InitializationFailed";
    case ErrorCode::OutOfHostMemory:
        return "OutOfHostMemory";
    case ErrorCode::OutOfDeviceMemory:
        return "OutOfDeviceMemory";
    case ErrorCode::DeviceLost:
        return "DeviceLost";
    case ErrorCode::SurfaceLost:
        return "SurfaceLost";
    case ErrorCode::OutOfDate:
        return "OutOfDate";
    case ErrorCode::Timeout:
        return "Timeout";
    }
    return "Unknown";
}

} // namespace rhi
