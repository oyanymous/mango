/**
 * @file
 * @brief VulkanInstance and VulkanInstanceHandle.
 */

#include "VulkanInstance.h"

#include <cstdio>
#include <string>
#include <vector>

#include "VulkanAdapter.h"
#include "VulkanSurface.h"

namespace rhi::vulkan {

namespace {

/// Lower-case name of a severity, for stderr output.
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
    return "?";
}

/// VK_EXT_debug_utils callback: converts the severity and forwards the message to the VulkanInstanceHandle passed as
/// user data.
VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                      VkDebugUtilsMessageTypeFlagsEXT,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                      void* user_data) {
    DebugSeverity rhi_severity = DebugSeverity::Verbose;
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        rhi_severity = DebugSeverity::Error;
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        rhi_severity = DebugSeverity::Warning;
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        rhi_severity = DebugSeverity::Info;
    }
    const auto* handle = static_cast<const VulkanInstanceHandle*>(user_data);
    handle->report(rhi_severity, data && data->pMessage ? data->pMessage : "");
    return VK_FALSE; // never abort the call that triggered the message
}

} // namespace

VulkanInstanceHandle::VulkanInstanceHandle(DebugCallback debug_callback) : m_debug_callback(std::move(debug_callback)) {
}

VulkanInstanceHandle::~VulkanInstanceHandle() {
    if (m_messenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);
    }
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
    }
}

void VulkanInstanceHandle::report(DebugSeverity severity, std::string_view message) const {
    if (m_debug_callback) {
        m_debug_callback(DebugMessage{severity, message});
        return;
    }
    std::fprintf(stderr, "[vulkan %s] %.*s\n", severityName(severity), static_cast<int>(message.size()),
                 message.data());
}

VkDebugUtilsMessengerCreateInfoEXT VulkanInstanceHandle::messengerCreateInfo() {
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debugMessengerCallback;
    info.pUserData = this;
    return info;
}

VulkanInstance::VulkanInstance(InstanceDesc desc, std::shared_ptr<VulkanInstanceHandle> handle)
    : m_desc(std::move(desc)), m_instance(std::move(handle)) {
}

AdapterList VulkanInstance::enumerateAdapters() {
    if (m_adapters_enumerated) {
        return m_adapters;
    }
    m_adapters_enumerated = true;

    std::uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_instance->get(), &count, nullptr);
    std::vector<VkPhysicalDevice> physical_devices(count);
    vkEnumeratePhysicalDevices(m_instance->get(), &count, physical_devices.data());
    physical_devices.resize(count);

    for (VkPhysicalDevice physical_device : physical_devices) {
        if (auto adapter = VulkanAdapter::create(m_instance, physical_device)) {
            m_adapters.push_back(std::move(*adapter));
        }
    }
    return m_adapters;
}

Result<SurfaceRef> VulkanInstance::createSurface(SurfaceDesc desc) {
    return VulkanSurface::create(m_instance, std::move(desc));
}

} // namespace rhi::vulkan
