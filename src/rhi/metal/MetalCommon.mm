/**
 * @file
 * @brief Shared helpers for the Metal backend.
 */

#include "MetalCommon.h"

#include <cstdio>
#include <utility>

namespace rhi::metal {

namespace {

/**
 * @brief Lower-case name of a severity, for stderr output.
 */
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

} // namespace rhi::metal
