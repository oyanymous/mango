#pragma once

/// @file
/// rhi::vulkan::VulkanInstance and the shared VkInstance owner, VulkanInstanceHandle.

#include <memory>

#include "IInstance.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

/// Owns the VkInstance and its debug messenger, and forwards debug messages to the user's callback.
///
/// Shared (via `std::shared_ptr`) by the instance, its adapters and, later, devices, so the VkInstance
/// outlives everything created from it without the instance and its adapters owning each other.
class VulkanInstanceHandle {
public:
    /// Creates an empty handle; VulkanDeviceFactory fills in m_instance and m_messenger.
    /// @param debug_callback Receives debug messages; null prints them to stderr.
    VulkanInstanceHandle(DebugCallback debug_callback);
    /// Destroys the messenger, then the instance.
    ~VulkanInstanceHandle();

    VulkanInstanceHandle(const VulkanInstanceHandle&) = delete;
    VulkanInstanceHandle& operator=(const VulkanInstanceHandle&) = delete;

    /// The VkInstance.
    VkInstance get() const { return m_instance; }

    /// Delivers a message to the user's callback, or to stderr if there is none.
    /// @param severity Message severity.
    /// @param message Message text.
    void report(DebugSeverity severity, std::string_view message) const;

    /// Create info for the debug messenger: warnings and errors of every message type, routed to report().
    ///
    /// Also chained into VkInstanceCreateInfo so that messages from instance creation itself are caught.
    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo();

    VkInstance m_instance = VK_NULL_HANDLE;                ///< The instance; destroyed with the handle.
    VkDebugUtilsMessengerEXT m_messenger = VK_NULL_HANDLE; ///< The debug messenger, if VK_EXT_debug_utils exists.
private:
    DebugCallback m_debug_callback;
};

/// The Vulkan IInstance. Created by VulkanDeviceFactory::createInstance.
class VulkanInstance final : public IInstance {
public:
    /// Wraps an already created instance.
    /// @param desc The descriptor it was created with.
    /// @param handle Owner of the VkInstance; shared with the adapters.
    VulkanInstance(InstanceDesc desc, std::shared_ptr<VulkanInstanceHandle> handle);

    AdapterList enumerateAdapters() override;

    /// Not implemented yet (step 9 of docs/RHI_VULKAN_PLAN.md); returns ErrorCode::Unsupported.
    Result<SurfaceRef> createSurface(SurfaceDesc desc) override;
private:
    InstanceDesc m_desc;
    AdapterList m_adapters; // enumerated once, on first use
    bool m_adapters_enumerated = false;
    std::shared_ptr<VulkanInstanceHandle> m_instance;
};

} // namespace rhi::vulkan
