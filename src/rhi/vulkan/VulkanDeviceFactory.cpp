/**
 * @file
 * @brief VulkanDeviceFactory: Vulkan instance creation.
 */

#include "VulkanDeviceFactory.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <string_view>
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

/// Makes the loader use only the validation layer built by cmake/VulkanValidationLayers.cmake, so the pinned version is
/// used even when a Vulkan SDK registers its own. VK_LAYER_PATH replaces the system's explicit-layer search; if the
/// developer already set VK_LAYER_PATH or VK_ADD_LAYER_PATH, theirs wins.
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
const vk::LayerProperties* findLayer(const std::vector<vk::LayerProperties>& layers, std::string_view name) {
    auto it = std::find_if(layers.begin(), layers.end(),
                           [&](const vk::LayerProperties& layer) { return std::string_view(layer.layerName) == name; });
    return it == layers.end() ? nullptr : &*it;
}

/// True if @p extensions contains an extension called @p name.
bool hasExtension(const std::vector<vk::ExtensionProperties>& extensions, std::string_view name) {
    return std::any_of(extensions.begin(), extensions.end(), [&](const vk::ExtensionProperties& extension) {
        return std::string_view(extension.extensionName) == name;
    });
}


/// The loader's instance-level API version; Vulkan 1.0 loaders lack vkEnumerateInstanceVersion.
std::uint32_t instanceVersion(const vk::raii::Context& context) {
    if (!context.getDispatcher()->vkEnumerateInstanceVersion) {
        return VK_API_VERSION_1_0;
    }
    auto version = context.enumerateInstanceVersion();
    return version.result == vk::Result::eSuccess ? version.value : VK_API_VERSION_1_0;
}

} // namespace

bool VulkanDeviceFactory::isLoaderAvailable() {
    // Opened once and kept open, so later checks and vk::raii::Context's own open of the same library are cheap.
    static const vk::detail::DynamicLoader loader;
    return loader.success() && loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr") != nullptr;
}

Result<InstanceRef> VulkanDeviceFactory::createInstance(InstanceDesc desc) {
    if (!isLoaderAvailable()) {
        return Error{ErrorCode::BackendUnavailable, "Vulkan loader not found (is a Vulkan GPU driver installed?)"};
    }

    auto handle = std::make_shared<VulkanInstanceHandle>(desc.debug_callback);
    const vk::raii::Context& context = handle->m_context;
    if (instanceVersion(context) < kApiVersion) {
        return Error{ErrorCode::BackendUnavailable, "the Vulkan loader is older than Vulkan 1.2"};
    }

    std::vector<const char*> layers;
    if (desc.enable_validation) {
        useBuiltLayerPath();
        const std::vector<vk::LayerProperties> available_layers = context.enumerateInstanceLayerProperties().value;
        if (const vk::LayerProperties* layer = findLayer(available_layers, kValidationLayer)) {
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

    const std::vector<vk::ExtensionProperties> available = context.enumerateInstanceExtensionProperties(nullptr).value;
    std::vector<const char*> extensions;
    vk::InstanceCreateFlags flags;

    const bool debug_utils = hasExtension(available, vk::EXTDebugUtilsExtensionName);
    if (debug_utils) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    // Surface extensions, so the instance can present. Missing ones only disable presentation.
    const char* surface_extensions[] = {
        vk::KHRSurfaceExtensionName,
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
    if (hasExtension(available, vk::KHRPortabilityEnumerationExtensionName)) {
        extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
        flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    }

    const vk::ApplicationInfo app_info{
        .pApplicationName = desc.application_name.c_str(),
        .applicationVersion = desc.application_version,
        .pEngineName = "Mango",
        .apiVersion = kApiVersion,
    };

    const vk::DebugUtilsMessengerCreateInfoEXT messenger_info = handle->messengerCreateInfo();

    const vk::InstanceCreateInfo create_info{
        .pNext = debug_utils ? &messenger_info : nullptr,
        .flags = flags,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    Result<vk::raii::Instance> instance = check(context.createInstance(create_info), "vkCreateInstance");
    if (!instance) {
        return instance.error();
    }
    handle->m_instance = std::move(instance).value();

    // Transitional: the files not yet ported to vk::raii call Vulkan through volk's global function pointers.
    volkInitializeCustom(context.getDispatcher()->vkGetInstanceProcAddr);
    volkLoadInstance(handle->get());

    if (debug_utils) {
        Result<vk::raii::DebugUtilsMessengerEXT> messenger =
            check(handle->m_instance.createDebugUtilsMessengerEXT(messenger_info), "vkCreateDebugUtilsMessengerEXT");
        if (!messenger) {
            return messenger.error();
        }
        handle->m_messenger = std::move(messenger).value();
    }

    return InstanceRef(std::make_shared<VulkanInstance>(std::move(desc), std::move(handle)));
}

} // namespace rhi::vulkan
