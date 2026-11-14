#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

#include <type_traits>

#include "RHI.h"

#if defined(MANGO_RHI_VULKAN)
#include <volk.h>
#endif

namespace {

int g_failures = 0;

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

} // namespace

int main() {
    testResult();
    testTypes();
    testInterfaces();
    testBackends();

    if (g_failures != 0) {
        std::printf("%d check(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }
    std::printf("All checks passed\n");
    return EXIT_SUCCESS;
}
