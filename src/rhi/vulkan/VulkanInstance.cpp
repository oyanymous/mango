#include "VulkanInstance.h"

#include <cstdio>
#include <string>

#include "VulkanAdapter.h"
#include "VulkanSurface.h"

namespace rhi::vulkan {

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
    return "?";
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL debugMessengerCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                        vk::DebugUtilsMessageTypeFlagsEXT,
                                                        const vk::DebugUtilsMessengerCallbackDataEXT* data,
                                                        void* user_data) {
    using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    DebugSeverity rhi_severity = DebugSeverity::Verbose;
    if (severity == Severity::eError) {
        rhi_severity = DebugSeverity::Error;
    } else if (severity == Severity::eWarning) {
        rhi_severity = DebugSeverity::Warning;
    } else if (severity == Severity::eInfo) {
        rhi_severity = DebugSeverity::Info;
    }
    const auto* handle = static_cast<const VulkanInstanceHandle*>(user_data);
    handle->report(rhi_severity, data && data->pMessage ? data->pMessage : "");
    return VK_FALSE;
}

} // namespace

VulkanInstanceHandle::VulkanInstanceHandle(DebugCallback debug_callback) : m_debug_callback(std::move(debug_callback)) {
}

void VulkanInstanceHandle::report(DebugSeverity severity, std::string_view message) const {
    if (m_debug_callback) {
        m_debug_callback(DebugMessage{severity, message});
        return;
    }
    std::fprintf(stderr, "[vulkan %s] %.*s\n", severityName(severity), static_cast<int>(message.size()),
                 message.data());
}

vk::DebugUtilsMessengerCreateInfoEXT VulkanInstanceHandle::messengerCreateInfo() {
    using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using Type = vk::DebugUtilsMessageTypeFlagBitsEXT;
    return vk::DebugUtilsMessengerCreateInfoEXT{
        .messageSeverity = Severity::eWarning | Severity::eError,
        .messageType = Type::eGeneral | Type::eValidation | Type::ePerformance,
        .pfnUserCallback = debugMessengerCallback,
        .pUserData = this,
    };
}

VulkanInstance::VulkanInstance(InstanceDesc desc, std::shared_ptr<VulkanInstanceHandle> handle)
    : m_desc(std::move(desc)), m_instance(std::move(handle)) {
}

AdapterList VulkanInstance::enumerateAdapters() {
    if (m_adapters_enumerated) {
        return m_adapters;
    }
    m_adapters_enumerated = true;

    auto physical_devices = m_instance->m_instance.enumeratePhysicalDevices();
    for (const vk::raii::PhysicalDevice& physical_device : physical_devices.value) {
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
