#pragma once

/**
 * @file
 * @brief Shared helpers for the Metal backend. Objective-C++ only; never include from a public RHI header.
 */

#import <Metal/Metal.h>

#include <string_view>

#include "IInstance.h"

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

} // namespace rhi::metal
