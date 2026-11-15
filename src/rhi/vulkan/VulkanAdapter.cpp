#include "VulkanAdapter.h"

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "VulkanDevice.h"
#include "VulkanQueue.h"

namespace rhi::vulkan {

namespace {

AdapterType toAdapterType(vk::PhysicalDeviceType type) {
    switch (type) {
    case vk::PhysicalDeviceType::eDiscreteGpu:
        return AdapterType::DiscreteGpu;
    case vk::PhysicalDeviceType::eIntegratedGpu:
        return AdapterType::IntegratedGpu;
    case vk::PhysicalDeviceType::eVirtualGpu:
        return AdapterType::VirtualGpu;
    case vk::PhysicalDeviceType::eCpu:
        return AdapterType::Cpu;
    default:
        return AdapterType::Unknown;
    }
}

Limits toLimits(const vk::PhysicalDeviceLimits& vk) {
    Limits limits;
    limits.max_texture_dimension_2d = vk.maxImageDimension2D;
    limits.max_texture_array_layers = vk.maxImageArrayLayers;
    limits.max_bind_groups = vk.maxBoundDescriptorSets;
    limits.max_uniform_buffer_range = vk.maxUniformBufferRange;
    limits.max_storage_buffer_range = vk.maxStorageBufferRange;
    limits.max_vertex_attributes = vk.maxVertexInputAttributes;
    limits.max_vertex_buffers = vk.maxVertexInputBindings;
    limits.max_color_attachments = vk.maxColorAttachments;
    std::copy_n(vk.maxComputeWorkGroupCount.begin(), 3, limits.max_compute_workgroup_count.begin());
    std::copy_n(vk.maxComputeWorkGroupSize.begin(), 3, limits.max_compute_workgroup_size.begin());
    limits.max_compute_invocations_per_workgroup = vk.maxComputeWorkGroupInvocations;
    limits.max_sampler_anisotropy = vk.maxSamplerAnisotropy;
    return limits;
}

} // namespace

std::optional<AdapterRef> VulkanAdapter::create(std::shared_ptr<VulkanInstanceHandle> instance,
                                                const vk::raii::PhysicalDevice& physical_device) {
    const vk::PhysicalDeviceProperties properties = physical_device.getProperties();
    if (properties.apiVersion < kApiVersion) {
        return std::nullopt;
    }

    const auto features =
        physical_device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan12Features>();
    const vk::PhysicalDeviceFeatures& features10 = features.get<vk::PhysicalDeviceFeatures2>().features;
    if (!features.get<vk::PhysicalDeviceVulkan12Features>().timelineSemaphore) {
        return std::nullopt;
    }

    Capabilities capabilities;
    capabilities.name = std::string(properties.deviceName);
    capabilities.type = toAdapterType(properties.deviceType);
    capabilities.vendor_id = properties.vendorID;
    capabilities.device_id = properties.deviceID;
    capabilities.api_version_major = VK_API_VERSION_MAJOR(properties.apiVersion);
    capabilities.api_version_minor = VK_API_VERSION_MINOR(properties.apiVersion);
    capabilities.api_version_patch = VK_API_VERSION_PATCH(properties.apiVersion);
    capabilities.features.sampler_anisotropy = features10.samplerAnisotropy == vk::True;
    capabilities.features.fill_mode_wireframe = features10.fillModeNonSolid == vk::True;
    capabilities.limits = toLimits(properties.limits);

    return AdapterRef(std::make_shared<VulkanAdapter>(std::move(instance), physical_device, std::move(capabilities)));
}

VulkanAdapter::VulkanAdapter(std::shared_ptr<VulkanInstanceHandle> instance, vk::raii::PhysicalDevice physical_device,
                             Capabilities capabilities)
    : m_instance(std::move(instance)),
      m_capabilities(std::move(capabilities)),
      m_physical_device(std::move(physical_device)) {
}

Capabilities VulkanAdapter::getCapabilities() const {
    return m_capabilities;
}

namespace {

int findQueueFamily(const std::vector<vk::QueueFamilyProperties>& families, vk::QueueFlags required,
                    vk::QueueFlags excluded) {
    for (std::size_t i = 0; i < families.size(); ++i) {
        const vk::QueueFlags flags = families[i].queueFlags;
        if (families[i].queueCount > 0 && (flags & required) == required && !(flags & excluded)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool hasDeviceExtension(const std::vector<vk::ExtensionProperties>& extensions, std::string_view name) {
    return std::any_of(extensions.begin(), extensions.end(),
                       [name](const vk::ExtensionProperties& e) { return std::string_view(e.extensionName) == name; });
}

} // namespace

Result<DeviceRef> VulkanAdapter::createDevice(DeviceDesc desc) {
    if (desc.features.sampler_anisotropy && !m_capabilities.features.sampler_anisotropy) {
        return Error{ErrorCode::Unsupported, "sampler_anisotropy is not supported by " + m_capabilities.name};
    }
    if (desc.features.fill_mode_wireframe && !m_capabilities.features.fill_mode_wireframe) {
        return Error{ErrorCode::Unsupported, "fill_mode_wireframe is not supported by " + m_capabilities.name};
    }

    const std::vector<vk::QueueFamilyProperties> families = m_physical_device.getQueueFamilyProperties();

    using Queue = vk::QueueFlagBits;
    const int graphics = findQueueFamily(families, Queue::eGraphics | Queue::eCompute, {});
    if (graphics < 0) {
        return Error{ErrorCode::Unsupported, m_capabilities.name + " has no graphics+compute queue family"};
    }
    int compute = findQueueFamily(families, Queue::eCompute, Queue::eGraphics);
    int transfer = findQueueFamily(families, Queue::eTransfer, Queue::eGraphics | Queue::eCompute);
    compute = compute < 0 ? graphics : compute;
    transfer = transfer < 0 ? graphics : transfer;
    const std::array<int, 3> family_of = {graphics, compute, transfer};

    const float priority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queue_infos;
    for (int family : std::set<int>(family_of.begin(), family_of.end())) {
        queue_infos.push_back({
            .queueFamilyIndex = static_cast<std::uint32_t>(family),
            .queueCount = 1,
            .pQueuePriorities = &priority,
        });
    }

    const std::vector<vk::ExtensionProperties> available =
        m_physical_device.enumerateDeviceExtensionProperties(nullptr).value;

    std::vector<const char*> extensions;

    const bool swapchain = hasDeviceExtension(available, vk::KHRSwapchainExtensionName);
    if (swapchain) {
        extensions.push_back(vk::KHRSwapchainExtensionName);
    }

    if (hasDeviceExtension(available, "VK_KHR_portability_subset")) {
        extensions.push_back("VK_KHR_portability_subset");
    }

    vk::PhysicalDeviceVulkan12Features features12{.timelineSemaphore = vk::True};
    const vk::PhysicalDeviceFeatures2 features{
        .pNext = &features12,
        .features =
            {
                .fillModeNonSolid = desc.features.fill_mode_wireframe ? vk::True : vk::False,
                .samplerAnisotropy = desc.features.sampler_anisotropy ? vk::True : vk::False,
            },
    };

    const vk::DeviceCreateInfo create_info{
        .pNext = &features,
        .queueCreateInfoCount = static_cast<std::uint32_t>(queue_infos.size()),
        .pQueueCreateInfos = queue_infos.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    auto handle = std::make_shared<VulkanDeviceHandle>(m_instance, m_physical_device);
    handle->m_graphics_family = static_cast<std::uint32_t>(graphics);
    handle->m_swapchain_enabled = swapchain;
    Result<vk::raii::Device> device = check(m_physical_device.createDevice(create_info), "vkCreateDevice");
    if (!device) {
        return device.error();
    }

    handle->m_device = std::move(device).value();

    VmaVulkanFunctions vma_functions{};
    vma_functions.vkGetInstanceProcAddr = m_instance->m_context.getDispatcher()->vkGetInstanceProcAddr;
    vma_functions.vkGetDeviceProcAddr = handle->m_device.getDispatcher()->vkGetDeviceProcAddr;
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = kApiVersion;
    allocator_info.physicalDevice = static_cast<VkPhysicalDevice>(*handle->m_physical_device);
    allocator_info.device = static_cast<VkDevice>(*handle->m_device);
    allocator_info.instance = static_cast<VkInstance>(*m_instance->m_instance);
    allocator_info.pVulkanFunctions = &vma_functions;
    VmaAllocator allocator = VK_NULL_HANDLE;
    if (VkResult result = vmaCreateAllocator(&allocator_info, &allocator); result != VK_SUCCESS) {
        return toError(result, "vmaCreateAllocator");
    }
    handle->m_allocator.reset(allocator);

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
