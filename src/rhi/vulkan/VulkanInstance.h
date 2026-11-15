#pragma once

#include <memory>

#include "IInstance.h"
#include "VulkanCommon.h"

namespace rhi::vulkan {

class VulkanInstanceHandle {
public:
    explicit VulkanInstanceHandle(DebugCallback debug_callback);

    VulkanInstanceHandle(const VulkanInstanceHandle&) = delete;
    VulkanInstanceHandle& operator=(const VulkanInstanceHandle&) = delete;

    void report(DebugSeverity severity, std::string_view message) const;

    vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo();
private:
    DebugCallback m_debug_callback;
public:
    vk::raii::Context m_context;
    vk::raii::Instance m_instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT m_messenger = nullptr;
};

class VulkanInstance final : public IInstance {
public:
    VulkanInstance(InstanceDesc desc, std::shared_ptr<VulkanInstanceHandle> handle);

    AdapterList enumerateAdapters() override;

    Result<SurfaceRef> createSurface(SurfaceDesc desc) override;
private:
    InstanceDesc m_desc;
    AdapterList m_adapters;
    bool m_adapters_enumerated = false;
    std::shared_ptr<VulkanInstanceHandle> m_instance;
};

} // namespace rhi::vulkan
