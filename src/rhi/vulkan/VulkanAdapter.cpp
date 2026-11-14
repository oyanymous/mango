/// @file
/// VulkanAdapter: physical-device queries.

#include "VulkanAdapter.h"

#include <algorithm>

namespace rhi::vulkan {

namespace {

/// Maps a Vulkan device type to AdapterType.
AdapterType toAdapterType(VkPhysicalDeviceType type) {
    switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return AdapterType::DiscreteGpu;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return AdapterType::IntegratedGpu;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return AdapterType::VirtualGpu;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return AdapterType::Cpu;
    default:
        return AdapterType::Unknown;
    }
}

/// Copies the Vulkan limits the RHI exposes into Limits.
Limits toLimits(const VkPhysicalDeviceLimits& vk) {
    Limits limits;
    limits.max_texture_dimension_2d = vk.maxImageDimension2D;
    limits.max_texture_array_layers = vk.maxImageArrayLayers;
    limits.max_bind_groups = vk.maxBoundDescriptorSets;
    limits.max_uniform_buffer_range = vk.maxUniformBufferRange;
    limits.max_storage_buffer_range = vk.maxStorageBufferRange;
    limits.max_vertex_attributes = vk.maxVertexInputAttributes;
    limits.max_vertex_buffers = vk.maxVertexInputBindings;
    limits.max_color_attachments = vk.maxColorAttachments;
    std::copy_n(vk.maxComputeWorkGroupCount, 3, limits.max_compute_workgroup_count.begin());
    std::copy_n(vk.maxComputeWorkGroupSize, 3, limits.max_compute_workgroup_size.begin());
    limits.max_compute_invocations_per_workgroup = vk.maxComputeWorkGroupInvocations;
    limits.max_sampler_anisotropy = vk.maxSamplerAnisotropy;
    return limits;
}

} // namespace

std::optional<AdapterRef> VulkanAdapter::create(std::shared_ptr<VulkanInstanceHandle> instance,
                                                VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    if (properties.apiVersion < kApiVersion) {
        return std::nullopt;
    }

    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &features12;
    vkGetPhysicalDeviceFeatures2(physical_device, &features);
    if (!features12.timelineSemaphore) {
        return std::nullopt; // ISemaphore's Timeline kind and fence bookkeeping rely on it
    }

    Capabilities capabilities;
    capabilities.name = properties.deviceName;
    capabilities.type = toAdapterType(properties.deviceType);
    capabilities.vendor_id = properties.vendorID;
    capabilities.device_id = properties.deviceID;
    capabilities.api_version_major = VK_API_VERSION_MAJOR(properties.apiVersion);
    capabilities.api_version_minor = VK_API_VERSION_MINOR(properties.apiVersion);
    capabilities.api_version_patch = VK_API_VERSION_PATCH(properties.apiVersion);
    capabilities.features.sampler_anisotropy = features.features.samplerAnisotropy == VK_TRUE;
    capabilities.features.fill_mode_wireframe = features.features.fillModeNonSolid == VK_TRUE;
    capabilities.limits = toLimits(properties.limits);

    return AdapterRef(std::make_shared<VulkanAdapter>(std::move(instance), physical_device, std::move(capabilities)));
}

VulkanAdapter::VulkanAdapter(std::shared_ptr<VulkanInstanceHandle> instance, VkPhysicalDevice physical_device,
                             Capabilities capabilities)
    : m_capabilities(std::move(capabilities)), m_physical_device(physical_device), m_instance(std::move(instance)) {
}

Capabilities VulkanAdapter::getCapabilities() const {
    return m_capabilities;
}

Result<DeviceRef> VulkanAdapter::createDevice(DeviceDesc) {
    return Error{ErrorCode::Unsupported, "VulkanAdapter::createDevice is implemented in step 4"};
}

} // namespace rhi::vulkan
