#pragma once

#include <memory>

#include "IInstance.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

/**
 * Owns the Vulkan instance and its debug messenger, and forwards debug messages to the user's callback.
 *
 * Shared (via `std::shared_ptr`) by the instance, its adapters, surfaces and devices, so the VkInstance outlives
 * everything created from it without the instance and its adapters owning each other.
 */
class VulkanInstanceHandle {
public:
    /**
     * Opens the Vulkan loader (kept open for the handle's life); VulkanDeviceFactory fills in m_instance and
     * m_messenger.
     *
     * @warning Only construct one after VulkanDeviceFactory::isLoaderAvailable() returned true: vk::raii::Context
     *          does not check that the library loaded.
     * @param debug_callback Receives debug messages; null prints them to stderr.
     */
    explicit VulkanInstanceHandle(DebugCallback debug_callback);

    VulkanInstanceHandle(const VulkanInstanceHandle&) = delete;
    VulkanInstanceHandle& operator=(const VulkanInstanceHandle&) = delete;

    /// Delivers a message to the user's callback, or to stderr if there is none.
    void report(DebugSeverity severity, std::string_view message) const;

    /**
     * Create info for the debug messenger: warnings and errors of every message type, routed to report().
     *
     * Also chained into vk::InstanceCreateInfo so that messages from instance creation and destruction are caught.
     */
    vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo();
private:
    // Declared first, so it is destroyed last: destroying the instance can still report messages.
    DebugCallback m_debug_callback;
public:
    vk::raii::Context m_context;
    vk::raii::Instance m_instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT m_messenger = nullptr; /**< Null without VK_EXT_debug_utils. */
};

/// The Vulkan IInstance. Created by VulkanDeviceFactory::createInstance.
class VulkanInstance final : public IInstance {
public:
    /**
     * Wraps an already created instance.
     *
     * @param handle Owner of the VkInstance; shared with the adapters.
     */
    VulkanInstance(InstanceDesc desc, std::shared_ptr<VulkanInstanceHandle> handle);

    AdapterList enumerateAdapters() override;

    /// Creates a VulkanSurface for a Win32 window (or, untested, a CAMetalLayer).
    Result<SurfaceRef> createSurface(SurfaceDesc desc) override;
private:
    InstanceDesc m_desc;
    AdapterList m_adapters; // enumerated once, on first use
    bool m_adapters_enumerated = false;
    std::shared_ptr<VulkanInstanceHandle> m_instance;
};

} // namespace rhi::vulkan
