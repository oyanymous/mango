#include "MetalCommon.h"

#include <cstdio>
#include <utility>

namespace rhi::metal {

namespace {

const char* severityName(DebugSeverity severity) {
    switch (severity) {
    case DebugSeverity::Verbose:
        return "verbose";
    case DebugSeverity::Info:
        return "info";
    case DebugSeverity::Warning:
        return "warning";
    case DebugSeverity::Error:
        return "error";
    }
    return "unknown";
}

} // namespace

MetalDebugReporter::MetalDebugReporter(DebugCallback debug_callback) : m_debug_callback(std::move(debug_callback)) {
}

void MetalDebugReporter::report(DebugSeverity severity, std::string_view message) const {
    if (m_debug_callback) {
        m_debug_callback(DebugMessage{severity, message});
        return;
    }
    std::fprintf(stderr, "[metal %s] %.*s\n", severityName(severity), static_cast<int>(message.size()), message.data());
}

NSString* toNSString(const std::string& text) {
    return text.empty() ? nil : [NSString stringWithUTF8String:text.c_str()];
}

MTLPixelFormat toMTLPixelFormat(Format format) {
    switch (format) {
    case Format::Undefined:
        return MTLPixelFormatInvalid;
    case Format::R8Unorm:
        return MTLPixelFormatR8Unorm;
    case Format::RG8Unorm:
        return MTLPixelFormatRG8Unorm;
    case Format::RGBA8Unorm:
        return MTLPixelFormatRGBA8Unorm;
    case Format::RGBA8Srgb:
        return MTLPixelFormatRGBA8Unorm_sRGB;
    case Format::BGRA8Unorm:
        return MTLPixelFormatBGRA8Unorm;
    case Format::BGRA8Srgb:
        return MTLPixelFormatBGRA8Unorm_sRGB;
    case Format::R16Float:
        return MTLPixelFormatR16Float;
    case Format::RG16Float:
        return MTLPixelFormatRG16Float;
    case Format::RGBA16Float:
        return MTLPixelFormatRGBA16Float;
    case Format::R32Uint:
        return MTLPixelFormatR32Uint;
    case Format::R32Float:
        return MTLPixelFormatR32Float;
    case Format::RG32Float:
        return MTLPixelFormatRG32Float;
    case Format::RGB32Float:
        return MTLPixelFormatInvalid;
    case Format::RGBA32Float:
        return MTLPixelFormatRGBA32Float;
    case Format::D16Unorm:
        return MTLPixelFormatDepth16Unorm;
    case Format::D24UnormS8Uint:
        return MTLPixelFormatDepth24Unorm_Stencil8;
    case Format::D32Float:
        return MTLPixelFormatDepth32Float;
    case Format::D32FloatS8Uint:
        return MTLPixelFormatDepth32Float_Stencil8;
    }
    return MTLPixelFormatInvalid;
}

Format fromMTLPixelFormat(MTLPixelFormat format) {
    switch (format) {
    case MTLPixelFormatBGRA8Unorm:
        return Format::BGRA8Unorm;
    case MTLPixelFormatBGRA8Unorm_sRGB:
        return Format::BGRA8Srgb;
    case MTLPixelFormatRGBA8Unorm:
        return Format::RGBA8Unorm;
    case MTLPixelFormatRGBA8Unorm_sRGB:
        return Format::RGBA8Srgb;
    case MTLPixelFormatRGBA16Float:
        return Format::RGBA16Float;
    default:
        return Format::Undefined;
    }
}

std::uint32_t bytesPerTexel(Format format) {
    switch (format) {
    case Format::Undefined:
        return 0;
    case Format::R8Unorm:
        return 1;
    case Format::RG8Unorm:
    case Format::R16Float:
    case Format::D16Unorm:
        return 2;
    case Format::RGBA8Unorm:
    case Format::RGBA8Srgb:
    case Format::BGRA8Unorm:
    case Format::BGRA8Srgb:
    case Format::RG16Float:
    case Format::R32Uint:
    case Format::R32Float:
    case Format::D24UnormS8Uint:
    case Format::D32Float:
    case Format::D32FloatS8Uint:
        return 4;
    case Format::RGBA16Float:
    case Format::RG32Float:
        return 8;
    case Format::RGB32Float:
        return 12;
    case Format::RGBA32Float:
        return 16;
    }
    return 0;
}

bool isDepthFormat(Format format) {
    return format == Format::D16Unorm || format == Format::D24UnormS8Uint || format == Format::D32Float ||
           format == Format::D32FloatS8Uint;
}

bool hasStencil(Format format) {
    return format == Format::D24UnormS8Uint || format == Format::D32FloatS8Uint;
}

MTLCompareFunction toMTLCompareFunction(CompareOp op) {
    switch (op) {
    case CompareOp::Never:
        return MTLCompareFunctionNever;
    case CompareOp::Less:
        return MTLCompareFunctionLess;
    case CompareOp::Equal:
        return MTLCompareFunctionEqual;
    case CompareOp::LessOrEqual:
        return MTLCompareFunctionLessEqual;
    case CompareOp::Greater:
        return MTLCompareFunctionGreater;
    case CompareOp::NotEqual:
        return MTLCompareFunctionNotEqual;
    case CompareOp::GreaterOrEqual:
        return MTLCompareFunctionGreaterEqual;
    case CompareOp::Always:
        return MTLCompareFunctionAlways;
    }
    return MTLCompareFunctionAlways;
}

} // namespace rhi::metal
