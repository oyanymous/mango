#pragma once

/**
 * @file
 * @brief Shared helpers for the Metal backend. Objective-C++ only; never include from a public RHI header.
 */

#import <Metal/Metal.h>

#include <memory>
#include <string>
#include <string_view>

#include "IInstance.h"
#include "RHITypes.h"

/**
 * @brief The Metal implementation of the RHI.
 *
 * @details
 * Everything here is private to the RHI library; applications use the interfaces in namespace rhi.
 * Sources are Objective-C++ compiled with ARC, so `id<MTL…>` members are released automatically. Headers that
 * hold Metal objects are included only from `.mm` files; MetalDeviceFactory.h, included from C++, stays plain C++.
 */
namespace rhi::metal {

/**
 * @brief PCI vendor ID reported for Apple GPUs.
 */
inline constexpr std::uint32_t kAppleVendorId = 0x106B;

/**
 * @brief Delivers debug messages to the InstanceDesc callback. Shared by an instance and its adapters.
 */
class MetalDebugReporter {
public:
    /**
     * @brief Creates a reporter.
     *
     * @param debug_callback Receives the messages; null prints them to stderr.
     */
    explicit MetalDebugReporter(DebugCallback debug_callback);

    /**
     * @brief Delivers a message to the callback, or to stderr if there is none.
     *
     * @param severity Message severity.
     * @param message Message text.
     */
    void report(DebugSeverity severity, std::string_view message) const;
private:
    DebugCallback m_debug_callback;
};

/**
 * @brief What every object of a device shares: the `MTLDevice` and the instance's debug reporter.
 *
 * @details
 * Held through a `std::shared_ptr` by the device and by every object it creates, so objects stay valid after the
 * DeviceRef is dropped (ARC releases the `MTLDevice` with the last of them).
 */
struct MetalDeviceContext {
    id<MTLDevice> device = nil;                   /**< The GPU. */
    std::shared_ptr<MetalDebugReporter> reporter; /**< Where errors and validation messages go. */
};

/**
 * @brief Converts a UTF-8 string to an `NSString`, e.g. for a Metal object's label.
 *
 * @param text UTF-8 text.
 * @return The string, or nil if @p text is empty (Metal treats a nil label as "no label").
 */
NSString* toNSString(const std::string& text);

/**
 * @brief The Metal pixel format of a Format; `MTLPixelFormatInvalid` for Format::Undefined or a format without a
 * Metal pixel format (RGB32Float, which exists only as a vertex format).
 */
MTLPixelFormat toMTLPixelFormat(Format format);

/**
 * @brief The Format of a Metal pixel format; Format::Undefined if the RHI has none.
 */
Format fromMTLPixelFormat(MTLPixelFormat format);

/**
 * @brief Bytes per texel in buffer copies; for depth/stencil formats, of the depth plane only (as copied).
 */
std::uint32_t bytesPerTexel(Format format);

/**
 * @brief True for D16Unorm, D24UnormS8Uint, D32Float and D32FloatS8Uint.
 */
bool isDepthFormat(Format format);

/**
 * @brief True for D24UnormS8Uint and D32FloatS8Uint.
 */
bool hasStencil(Format format);

/**
 * @brief The Metal compare function of a CompareOp.
 */
MTLCompareFunction toMTLCompareFunction(CompareOp op);

} // namespace rhi::metal
