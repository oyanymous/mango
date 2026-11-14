/// @file
/// VulkanDeviceFactory: Vulkan instance creation.

#include "VulkanDeviceFactory.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "VulkanInstance.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace rhi::vulkan {

namespace {

/// Name of the Khronos validation layer.
constexpr const char* kValidationLayer = "VK_LAYER_KHRONOS_validation";

/// Makes the loader use only the validation layer built by cmake/VulkanValidationLayers.cmake, so the
/// pinned version is used even when a Vulkan SDK registers its own. VK_LAYER_PATH replaces the system's
/// explicit-layer search; if the developer already set VK_LAYER_PATH or VK_ADD_LAYER_PATH, theirs wins.
void useBuiltLayerPath() {
#if defined(MANGO_VULKAN_LAYER_PATH)
    if (std::getenv("VK_LAYER_PATH") != nullptr || std::getenv("VK_ADD_LAYER_PATH") != nullptr) {
        return;
    }
#if defined(_WIN32)
    // SetEnvironmentVariable updates the process environment the (MSVC-built) loader reads.
    SetEnvironmentVariableA("VK_LAYER_PATH", MANGO_VULKAN_LAYER_PATH);
#else
    setenv("VK_LAYER_PATH", MANGO_VULKAN_LAYER_PATH, 0);
#endif
#endif
}

/// Returns the layer called @p name from @p layers, or null.
const VkLayerProperties* findLayer(const std::vector<VkLayerProperties>& layers, const char* name) {
    auto it = std::find_if(layers.begin(), layers.end(),
                           [&](const VkLayerProperties& layer) { return std::strcmp(layer.layerName, name) == 0; });
    return it == layers.end() ? nullptr : &*it;
}

/// True if @p extensions contains an extension called @p name.
bool hasExtension(const std::vector<VkExtensionProperties>& extensions, const char* name) {
    return std::any_of(extensions.begin(), extensions.end(), [&](const VkExtensionProperties& extension) {
        return std::strcmp(extension.extensionName, name) == 0;
    });
}

/// The instance layers the loader can find.
std::vector<VkLayerProperties> instanceLayers() {
    std::uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    layers.resize(count);
    return layers;
}

/// The instance extensions the loader and drivers provide.
std::vector<VkExtensionProperties> instanceExtensions() {
    std::uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
    extensions.resize(count);
    return extensions;
}

} // namespace

bool VulkanDeviceFactory::isLoaderAvailable() {
    // volkInitialize is cheap after the first successful call.
    return volkInitialize() == VK_SUCCESS;
}

Result<InstanceRef> VulkanDeviceFactory::createInstance(InstanceDesc desc) {
    if (VkResult result = volkInitialize(); result != VK_SUCCESS) {
        return Error{ErrorCode::BackendUnavailable, "Vulkan loader not found (is a Vulkan GPU driver installed?)"};
    }
    if (volkGetInstanceVersion() < kApiVersion) {
        return Error{ErrorCode::BackendUnavailable, "the Vulkan loader is older than Vulkan 1.2"};
    }

    auto handle = std::make_shared<VulkanInstanceHandle>(desc.debug_callback);

    std::vector<const char*> layers;
    if (desc.enable_validation) {
        useBuiltLayerPath();
        const std::vector<VkLayerProperties> available_layers = instanceLayers();
        if (const VkLayerProperties* layer = findLayer(available_layers, kValidationLayer)) {
            layers.push_back(kValidationLayer);
            handle->report(DebugSeverity::Info, std::string("using ") + kValidationLayer + " " +
                                                    std::to_string(VK_API_VERSION_MAJOR(layer->specVersion)) + "." +
                                                    std::to_string(VK_API_VERSION_MINOR(layer->specVersion)) + "." +
                                                    std::to_string(VK_API_VERSION_PATCH(layer->specVersion)));
        } else {
            handle->report(DebugSeverity::Warning,
                           "validation requested but VK_LAYER_KHRONOS_validation was not found; continuing without it");
        }
    }

    const std::vector<VkExtensionProperties> available = instanceExtensions();
    std::vector<const char*> extensions;
    VkInstanceCreateFlags flags = 0;

    const bool debug_utils = hasExtension(available, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    if (debug_utils) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Surface extensions, so the instance can present (step 9). Missing ones only disable presentation.
    const char* surface_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
        "VK_KHR_win32_surface",
#elif defined(__APPLE__)
        "VK_EXT_metal_surface",
#endif
    };
    for (const char* extension : surface_extensions) {
        if (hasExtension(available, extension)) {
            extensions.push_back(extension);
        }
    }

    // MoltenVK is a "portability" implementation; it is only listed when the instance opts in.
    if (hasExtension(available, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = desc.application_name.c_str();
    app_info.applicationVersion = desc.application_version;
    app_info.pEngineName = "Mango";
    app_info.apiVersion = kApiVersion;

    VkDebugUtilsMessengerCreateInfoEXT messenger_info = handle->messengerCreateInfo();

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = debug_utils ? &messenger_info : nullptr;
    create_info.flags = flags;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
    create_info.ppEnabledLayerNames = layers.data();
    create_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    if (VkResult result = vkCreateInstance(&create_info, nullptr, &handle->m_instance); result != VK_SUCCESS) {
        handle->m_instance = VK_NULL_HANDLE;
        return toError(result, "vkCreateInstance");
    }
    volkLoadInstance(handle->m_instance);

    if (debug_utils) {
        if (VkResult result =
                vkCreateDebugUtilsMessengerEXT(handle->m_instance, &messenger_info, nullptr, &handle->m_messenger);
            result != VK_SUCCESS) {
            return toError(result, "vkCreateDebugUtilsMessengerEXT");
        }
    }

    return InstanceRef(std::make_shared<VulkanInstance>(std::move(desc), std::move(handle)));
}

} // namespace rhi::vulkan
