/**
 * @file
 * @brief VulkanAdapter: physical-device queries.
 */

#include "VulkanAdapter.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <set>
#include <vector>

#include "VulkanDevice.h"
#include "VulkanQueue.h"

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

namespace {

/// Index of the first queue family that has all of @p required and none of @p excluded, or -1.
int findQueueFamily(const std::vector<VkQueueFamilyProperties>& families, VkQueueFlags required,
                    VkQueueFlags excluded) {
    for (std::size_t i = 0; i < families.size(); ++i) {
        const VkQueueFlags flags = families[i].queueFlags;
        if (families[i].queueCount > 0 && (flags & required) == required && (flags & excluded) == 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

/// Whether the physical device exposes a device extension.
bool hasDeviceExtension(const std::vector<VkExtensionProperties>& extensions, const char* name) {
    return std::any_of(extensions.begin(), extensions.end(),
                       [name](const VkExtensionProperties& e) { return std::strcmp(e.extensionName, name) == 0; });
}

} // namespace

Result<DeviceRef> VulkanAdapter::createDevice(DeviceDesc desc) {
    if (desc.features.sampler_anisotropy && !m_capabilities.features.sampler_anisotropy) {
        return Error{ErrorCode::Unsupported, "sampler_anisotropy is not supported by " + m_capabilities.name};
    }
    if (desc.features.fill_mode_wireframe && !m_capabilities.features.fill_mode_wireframe) {
        return Error{ErrorCode::Unsupported, "fill_mode_wireframe is not supported by " + m_capabilities.name};
    }

    std::uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &family_count, nullptr);
    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &family_count, families.data());

    // Vulkan guarantees a graphics+compute family on any device with graphics. Compute and transfer prefer
    // dedicated families (async work) and fall back to the graphics family, as IDevice::getQueue documents.
    const int graphics = findQueueFamily(families, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0);
    if (graphics < 0) {
        return Error{ErrorCode::Unsupported, m_capabilities.name + " has no graphics+compute queue family"};
    }
    int compute = findQueueFamily(families, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT);
    int transfer = findQueueFamily(families, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    compute = compute < 0 ? graphics : compute;
    transfer = transfer < 0 ? graphics : transfer;
    const std::array<int, 3> family_of = {graphics, compute, transfer}; // indexed by QueueType

    const float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    for (int family : std::set<int>(family_of.begin(), family_of.end())) {
        VkDeviceQueueCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = static_cast<std::uint32_t>(family);
        info.queueCount = 1;
        info.pQueuePriorities = &priority;
        queue_infos.push_back(info);
    }

    std::uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> available(extension_count);
    vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &extension_count, available.data());

    std::vector<const char*> extensions;
    // Without the extension only presentation is unavailable (createSwapchain returns Unsupported).
    const bool swapchain = hasDeviceExtension(available, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    if (swapchain) {
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    // The spec requires enabling it on portability implementations (MoltenVK) whenever it is exposed.
    if (hasDeviceExtension(available, "VK_KHR_portability_subset")) {
        extensions.push_back("VK_KHR_portability_subset");
    }

    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.timelineSemaphore = VK_TRUE; // checked in create()
    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &features12;
    features.features.samplerAnisotropy = desc.features.sampler_anisotropy ? VK_TRUE : VK_FALSE;
    features.features.fillModeNonSolid = desc.features.fill_mode_wireframe ? VK_TRUE : VK_FALSE;

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext = &features;
    create_info.queueCreateInfoCount = static_cast<std::uint32_t>(queue_infos.size());
    create_info.pQueueCreateInfos = queue_infos.data();
    create_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    auto handle = std::make_shared<VulkanDeviceHandle>(m_instance);
    handle->m_physical_device = m_physical_device;
    handle->m_graphics_family = static_cast<std::uint32_t>(graphics);
    handle->m_swapchain_enabled = swapchain;
    if (VkResult result = vkCreateDevice(m_physical_device, &create_info, nullptr, &handle->m_device);
        result != VK_SUCCESS) {
        handle->m_device = VK_NULL_HANDLE;
        return toError(result, "vkCreateDevice");
    }
    // No volkLoadDevice: it would bind volk's global function pointers to this one device. The instance-level
    // pointers dispatch through the loader and work for any number of devices.

    VmaVulkanFunctions vma_functions{};
    vma_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vma_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = kApiVersion;
    allocator_info.physicalDevice = m_physical_device;
    allocator_info.device = handle->m_device;
    allocator_info.instance = m_instance->get();
    allocator_info.pVulkanFunctions = &vma_functions;
    if (VkResult result = vmaCreateAllocator(&allocator_info, &handle->m_allocator); result != VK_SUCCESS) {
        handle->m_allocator = VK_NULL_HANDLE;
        return toError(result, "vmaCreateAllocator");
    }

    // One queue object per family, shared by the types that fall back to it.
    std::array<QueueRef, 3> queues;
    for (std::size_t type = 0; type < queues.size(); ++type) {
        for (std::size_t earlier = 0; earlier < type; ++earlier) {
            if (family_of[earlier] == family_of[type]) {
                queues[type] = queues[earlier];
            }
        }
        if (!queues[type]) {
            Result<std::shared_ptr<VulkanQueue>> queue =
                VulkanQueue::create(handle, static_cast<QueueType>(type), static_cast<std::uint32_t>(family_of[type]));
            if (!queue) {
                return queue.error();
            }
            queues[type] = std::move(queue).value();
        }
    }

    return DeviceRef(std::make_shared<VulkanDevice>(std::move(desc), std::move(handle), std::move(queues)));
}

} // namespace rhi::vulkan
