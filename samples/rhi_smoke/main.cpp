/// @file
/// rhi_smoke: headless checks of the RHI, run by CTest.
///
/// Runs every check, prints `[ OK ]` / `[FAIL]` per check, and exits non-zero if any failed. With the Vulkan
/// backend it also creates a validated instance, lists the adapters, and fails on any validation warning.

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include <type_traits>

#include "RHI.h"

#if defined(MANGO_RHI_VULKAN)
#include <volk.h>
#endif

namespace {

int g_failures = 0; // failed checks so far

// Prints the outcome of one check and counts failures.
void check(bool condition, const char* what) {
    std::printf("[%s] %s\n", condition ? " OK " : "FAIL", what);
    if (!condition) {
        ++g_failures;
    }
}

rhi::Result<int> parsePositive(int value) {
    if (value <= 0) {
        return rhi::Error{rhi::ErrorCode::InvalidArgument, "value must be positive"};
    }
    return value;
}

rhi::Result<void> succeedIf(bool ok) {
    if (!ok) {
        return rhi::Error{rhi::ErrorCode::Unsupported, "not ok"};
    }
    return {};
}

void testResult() {
    auto good = parsePositive(42);
    check(good.hasValue() && *good == 42, "Result<int> holds a value");

    auto bad = parsePositive(-1);
    check(!bad && bad.error().code == rhi::ErrorCode::InvalidArgument, "Result<int> holds an error");
    check(bad.valueOr(7) == 7, "Result<int>::valueOr falls back on error");

    check(static_cast<bool>(succeedIf(true)), "Result<void> success");
    check(!succeedIf(false) && succeedIf(false).error().message == "not ok", "Result<void> error");

    rhi::Result<std::unique_ptr<int>> owned = std::make_unique<int>(5);
    std::unique_ptr<int> moved = std::move(owned).value();
    check(moved && *moved == 5, "Result<T> supports move-only values");

    check(rhi::toString(rhi::ErrorCode::OutOfDate) == "OutOfDate", "toString(ErrorCode)");
}

void testTypes() {
    check(rhi::Extent2D{1, 2} == rhi::Extent2D{1, 2}, "Extent2D equality");
    check(rhi::Extent3D{4, 4}.depth == 1, "Extent3D depth defaults to 1");
}

// Every RHI interface must be abstract and safe to delete through a base pointer.
template <typename... Interfaces>
constexpr bool allAreInterfaces() {
    return ((std::is_abstract_v<Interfaces> && std::has_virtual_destructor_v<Interfaces>) && ...);
}
static_assert(
    allAreInterfaces<rhi::IDeviceFactory, rhi::IInstance, rhi::IAdapter, rhi::IDevice, rhi::IQueue, rhi::ISurface,
                     rhi::ISwapchain, rhi::IBuffer, rhi::ITexture, rhi::ITextureView, rhi::ISampler, rhi::IShader,
                     rhi::IPipelineLayout, rhi::IPipeline, rhi::IBindGroupLayout, rhi::IBindGroup, rhi::IRenderPass,
                     rhi::IRenderTarget, rhi::ICommandBuffer, rhi::IRenderEncoder, rhi::IComputeEncoder,
                     rhi::ICopyEncoder, rhi::IFence, rhi::ISemaphore>());

// A minimal backend-free implementation, to prove the interfaces can be implemented and passed around.
class FakeFence final : public rhi::IFence {
public:
    rhi::Result<void> wait(rhi::Timeout) override {
        if (!m_signaled) {
            return rhi::Error{rhi::ErrorCode::Timeout, "never signaled"};
        }
        return {};
    }
    bool isSignaled() const override { return m_signaled; }
    rhi::Result<void> reset() override {
        m_signaled = false;
        return {};
    }

    bool m_signaled = true;
};

rhi::Result<rhi::FenceRef> createFakeFence(rhi::FenceDesc desc) {
    auto fence = std::make_shared<FakeFence>();
    fence->m_signaled = desc.signaled;
    return rhi::FenceRef(fence);
}

void testInterfaces() {
    auto fence = createFakeFence({.signaled = true, .label = "frame fence"});
    check(fence && (*fence)->isSignaled(), "interface implemented and returned through Result<FenceRef>");
    check((*fence)->reset() && !(*fence)->wait(rhi::Timeout(0)), "fake fence times out after reset");

    rhi::BufferDesc buffer{
        .size = 256,
        .usage = rhi::BufferUsage::Vertex | rhi::BufferUsage::CopyDst,
        .memory = rhi::MemoryLocation::GpuOnly,
        .label = "vertices",
    };
    check(rhi::hasFlag(buffer.usage, rhi::BufferUsage::Vertex) && !rhi::hasFlag(buffer.usage, rhi::BufferUsage::Index),
          "flag operators");
    check(rhi::hasFlag(rhi::ShaderStage::AllGraphics, rhi::ShaderStage::Fragment), "ShaderStage::AllGraphics");

    rhi::TextureViewDesc view{};
    check(!view.format && view.range.mip_level_count == rhi::kRemaining, "texture view defaults follow the texture");

    rhi::SurfaceDesc surface{.window = rhi::Win32WindowHandle{.hinstance = nullptr, .hwnd = nullptr}};
    check(std::holds_alternative<rhi::Win32WindowHandle>(surface.window), "native window handle variant");
}

void testBackends() {
    std::printf("Compiled backends:");
    for (rhi::Backend backend : rhi::compiledBackends()) {
        std::printf(" %s", std::string(rhi::toString(backend)).c_str());
    }
    std::printf("\n");

#if defined(MANGO_RHI_VULKAN)
    check(rhi::compiledBackends().size() == 1 && rhi::compiledBackends()[0] == rhi::Backend::Vulkan,
          "Vulkan backend is compiled in");

    std::printf("Vulkan headers: %u.%u.%u\n", VK_API_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE),
                VK_API_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE), VK_API_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE));

    // volk opens the loader that ships with the GPU driver (vulkan-1.dll / libvulkan).
    VkResult result = volkInitialize();
    check(result == VK_SUCCESS, "volk finds the Vulkan loader");
    if (result != VK_SUCCESS) {
        return;
    }

    std::uint32_t loader_version = 0;
    result = vkEnumerateInstanceVersion(&loader_version);
    check(result == VK_SUCCESS, "vkEnumerateInstanceVersion succeeds");
    if (result == VK_SUCCESS) {
        std::printf("Vulkan loader: %u.%u.%u\n", VK_API_VERSION_MAJOR(loader_version),
                    VK_API_VERSION_MINOR(loader_version), VK_API_VERSION_PATCH(loader_version));
        check(loader_version >= VK_API_VERSION_1_2, "Vulkan loader supports 1.2");
    }
#else
    check(rhi::compiledBackends().empty(), "no backend compiled in");
#endif
}

const char* adapterTypeName(rhi::AdapterType type) {
    switch (type) {
    case rhi::AdapterType::DiscreteGpu:
        return "discrete GPU";
    case rhi::AdapterType::IntegratedGpu:
        return "integrated GPU";
    case rhi::AdapterType::VirtualGpu:
        return "virtual GPU";
    case rhi::AdapterType::Cpu:
        return "CPU";
    case rhi::AdapterType::Unknown:
        break;
    }
    return "unknown";
}

void testInstance() {
#if defined(MANGO_RHI_VULKAN)
    rhi::RHIFactory factory;
    rhi::BackendList available = factory.availableBackends();
    check(available.size() == 1 && available[0] == rhi::Backend::Vulkan, "RHIFactory lists Vulkan as available");

    check(!factory.createInstance(rhi::Backend::Metal, {}) &&
              factory.createInstance(rhi::Backend::Metal, {}).error().code == rhi::ErrorCode::BackendUnavailable,
          "RHIFactory rejects a backend that is not compiled in");

    // Every warning or error from the driver or the validation layer counts as a failure.
    int problems = 0;
    std::string layer_in_use;
    rhi::InstanceDesc desc{
        .application_name = "rhi_smoke",
        .enable_validation = true,
        .debug_callback =
            [&problems, &layer_in_use](const rhi::DebugMessage& message) {
                if (message.severity == rhi::DebugSeverity::Warning || message.severity == rhi::DebugSeverity::Error) {
                    ++problems;
                }
                if (message.message.starts_with("using VK_LAYER_KHRONOS_validation")) {
                    layer_in_use = message.message;
                }
                std::printf("  [debug] %.*s\n", static_cast<int>(message.message.size()), message.message.data());
            },
    };

    {
        auto instance = factory.createInstance(rhi::Backend::Vulkan, desc);
        check(static_cast<bool>(instance),
              instance ? "Vulkan instance created with validation" : instance.error().message.c_str());
        if (!instance) {
            return;
        }

        rhi::AdapterList adapters = (*instance)->enumerateAdapters();
        check(!adapters.empty(), "at least one Vulkan 1.2 adapter");
        check((*instance)->enumerateAdapters().size() == adapters.size(), "adapters are enumerated once and cached");

        for (const rhi::AdapterRef& adapter : adapters) {
            rhi::Capabilities caps = adapter->getCapabilities();
            std::printf("Adapter: %s (%s), Vulkan %u.%u.%u, vendor 0x%04x device 0x%04x\n", caps.name.c_str(),
                        adapterTypeName(caps.type), caps.api_version_major, caps.api_version_minor,
                        caps.api_version_patch, caps.vendor_id, caps.device_id);
            std::printf("  max 2D texture %u, max bind groups %u, max colour attachments %u, anisotropy %s (%.0fx)\n",
                        caps.limits.max_texture_dimension_2d, caps.limits.max_bind_groups,
                        caps.limits.max_color_attachments, caps.features.sampler_anisotropy ? "yes" : "no",
                        caps.limits.max_sampler_anisotropy);
            std::printf("  compute workgroup size max %u x %u x %u, %u invocations\n",
                        caps.limits.max_compute_workgroup_size[0], caps.limits.max_compute_workgroup_size[1],
                        caps.limits.max_compute_workgroup_size[2], caps.limits.max_compute_invocations_per_workgroup);
            check(!caps.name.empty() && caps.limits.max_texture_dimension_2d >= 4096, "adapter capabilities filled in");
        }

        std::vector<rhi::DeviceRef> devices;
        for (const rhi::AdapterRef& adapter : adapters) {
            rhi::Capabilities caps = adapter->getCapabilities();
            auto device = adapter->createDevice({.features = caps.features, .label = "smoke device"});
            check(static_cast<bool>(device),
                  device ? "device created with every supported feature" : device.error().message.c_str());
            if (!device) {
                continue;
            }
            rhi::QueueRef graphics = (*device)->getQueue(rhi::QueueType::Graphics);
            rhi::QueueRef compute = (*device)->getQueue(rhi::QueueType::Compute);
            rhi::QueueRef transfer = (*device)->getQueue(rhi::QueueType::Transfer);
            check(graphics && compute && transfer, "device has graphics, compute and transfer queues");
            check((*device)->getQueue(rhi::QueueType::Graphics) == graphics, "getQueue returns the same queue");
            std::printf("  queues: compute %s, transfer %s\n",
                        compute == graphics ? "shared with graphics" : "dedicated",
                        transfer == graphics ? "shared with graphics" : "dedicated");
            devices.push_back(*device);
        }

        // Adapters keep the VkInstance alive on their own; dropping the instance first must be safe.
        instance = rhi::Error{};
        check(adapters.empty() || !adapters[0]->getCapabilities().name.empty(), "adapters outlive their instance");

        // Devices and queues keep the VkDevice and VkInstance alive; validation reports leaks on destruction.
        adapters.clear();
        rhi::QueueRef queue = devices.empty() ? nullptr : devices[0]->getQueue(rhi::QueueType::Graphics);
        devices.clear();
        check(devices.empty() && (adapters.empty() || queue), "queues outlive their device");
    }

#if defined(MANGO_VULKAN_VALIDATION)
    // The layer built from source is on the same pinned SDK tag as the Vulkan headers.
    const std::string expected_layer = "using VK_LAYER_KHRONOS_validation " +
                                       std::to_string(VK_API_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE)) + "." +
                                       std::to_string(VK_API_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE)) + "." +
                                       std::to_string(VK_API_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE));
    check(layer_in_use == expected_layer, "the pinned validation layer built from source is the one in use");
    check(problems == 0, "no validation warnings or errors");
#else
    std::printf("Validation layer not built (MANGO_VULKAN_VALIDATION=OFF); %d warning(s)/error(s)\n", problems);
#endif
#endif
}

} // namespace

int main() {
    testResult();
    testTypes();
    testInterfaces();
    testBackends();
    testInstance();

    if (g_failures != 0) {
        std::printf("%d check(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }
    std::printf("All checks passed\n");
    return EXIT_SUCCESS;
}
