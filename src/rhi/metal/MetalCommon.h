#pragma once

#import <Metal/Metal.h>

#include <memory>
#include <string>
#include <string_view>

#include "IInstance.h"
#include "RHITypes.h"

namespace rhi::metal {

inline constexpr std::uint32_t kAppleVendorId = 0x106B;

class MetalDebugReporter {
public:
    explicit MetalDebugReporter(DebugCallback debug_callback);

    void report(DebugSeverity severity, std::string_view message) const;
private:
    DebugCallback m_debug_callback;
};

struct MetalDeviceContext {
    id<MTLDevice> device = nil;
    std::shared_ptr<MetalDebugReporter> reporter;
};

NSString* toNSString(const std::string& text);

MTLPixelFormat toMTLPixelFormat(Format format);

Format fromMTLPixelFormat(MTLPixelFormat format);

std::uint32_t bytesPerTexel(Format format);

bool isDepthFormat(Format format);

bool hasStencil(Format format);

MTLCompareFunction toMTLCompareFunction(CompareOp op);

} // namespace rhi::metal
