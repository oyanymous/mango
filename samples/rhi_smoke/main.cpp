#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <type_traits>

#include "RHI.h"

#if defined(MANGO_RHI_VULKAN)

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_DEFAULT_DISPATCHER
#include <vulkan/vulkan.hpp>
#endif
#if defined(MANGO_RHI_VULKAN) || defined(MANGO_RHI_METAL)
#include "fill.comp.h"
#include "fullscreen.vert.h"
#include "red.frag.h"
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

    std::vector<rhi::Backend> expected;
#if defined(MANGO_RHI_VULKAN)
    expected.push_back(rhi::Backend::Vulkan);
#endif
#if defined(MANGO_RHI_METAL)
    expected.push_back(rhi::Backend::Metal);
#endif
    check(std::ranges::equal(rhi::compiledBackends(), expected), "compiled backends match the MANGO_RHI_* options");

#if defined(MANGO_RHI_VULKAN)
    std::printf("Vulkan headers: %u.%u.%u\n", VK_API_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE),
                VK_API_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE), VK_API_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE));

    const vk::detail::DynamicLoader loader;
    const auto get_instance_proc_addr =
        loader.success() ? loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr") : nullptr;
#if defined(__APPLE__)

    if (!get_instance_proc_addr) {
        std::printf("Vulkan loader not found (no MoltenVK); skipping the Vulkan checks\n");
        return;
    }
#endif
    check(get_instance_proc_addr != nullptr, "the Vulkan loader is found");
    if (!get_instance_proc_addr) {
        return;
    }
    const auto enumerate_instance_version =
        reinterpret_cast<PFN_vkEnumerateInstanceVersion>(get_instance_proc_addr(nullptr, "vkEnumerateInstanceVersion"));
    check(enumerate_instance_version != nullptr, "the loader has vkEnumerateInstanceVersion (Vulkan 1.1+)");
    if (!enumerate_instance_version) {
        return;
    }

    std::uint32_t loader_version = 0;
    const VkResult result = enumerate_instance_version(&loader_version);
    check(result == VK_SUCCESS, "vkEnumerateInstanceVersion succeeds");
    if (result == VK_SUCCESS) {
        std::printf("Vulkan loader: %u.%u.%u\n", VK_API_VERSION_MAJOR(loader_version),
                    VK_API_VERSION_MINOR(loader_version), VK_API_VERSION_PATCH(loader_version));
        check(loader_version >= VK_API_VERSION_1_2, "Vulkan loader supports 1.2");
    }
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

void testSync(rhi::IDevice& device) {
    const rhi::Timeout timeout = std::chrono::seconds(5);
    rhi::QueueRef queue = device.getQueue(rhi::QueueType::Graphics);

    auto signaled = device.createFence({.signaled = true});
    check(signaled && (*signaled)->isSignaled(), "fence created signaled");

    auto fence = device.createFence({});
    if (!fence) {
        check(false, fence.error().message.c_str());
        return;
    }
    check(!(*fence)->isSignaled(), "new fence is unsignaled");
    auto poll = (*fence)->wait(rhi::Timeout(0));
    check(!poll && poll.error().code == rhi::ErrorCode::Timeout, "polling an unsignaled fence times out");

    check(static_cast<bool>(queue->submit({.fence = *fence})), "empty submit with a fence");
    check((*fence)->wait(timeout) && (*fence)->isSignaled(), "fence signaled after the submit");
    check((*fence)->reset() && !(*fence)->isSignaled(), "fence reset");

    auto timeline = device.createSemaphore({.kind = rhi::SemaphoreKind::Timeline, .initial_value = 5});
    auto binary = device.createSemaphore({});
    if (!timeline || !binary) {
        check(false, "semaphores created");
        return;
    }
    check((*timeline)->getKind() == rhi::SemaphoreKind::Timeline && (*timeline)->getValue() == 5,
          "timeline semaphore starts at its initial value");
    check((*binary)->getKind() == rhi::SemaphoreKind::Binary && (*binary)->getValue() == 0, "binary semaphore");

    check(static_cast<bool>(queue->submit({.signals = {{*timeline, 6}, {*binary, 0}}})), "submit signaling semaphores");
    check(static_cast<bool>(queue->submit({
              .waits = {{*timeline, 6, rhi::PipelineStage::AllCommands}, {*binary, 0, rhi::PipelineStage::AllCommands}},
              .signals = {{*timeline, 7}},
              .fence = *fence,
          })),
          "submit waiting on semaphores");
    check(static_cast<bool>((*fence)->wait(timeout)), "fence after the semaphore chain");
    check((*timeline)->getValue() == 7, "timeline value advanced to 7");

    rhi::QueueRef compute = device.getQueue(rhi::QueueType::Compute);
    check((*fence)->reset() && queue->submit({.signals = {{*timeline, 8}}}) &&
              compute->submit({
                  .waits = {{*timeline, 8, rhi::PipelineStage::AllCommands}},
                  .signals = {{*timeline, 9}},
                  .fence = *fence,
              }),
          "cross-queue submits");
    check((*fence)->wait(timeout) && (*timeline)->getValue() == 9, "compute queue ran after the graphics signal");
}

void testBufferRoundTrip(rhi::IDevice& device) {
    constexpr std::uint64_t kSize = 64 * 1024;

    auto bad = device.createBuffer({.size = 0, .usage = rhi::BufferUsage::CopySrc});
    check(!bad && bad.error().code == rhi::ErrorCode::InvalidArgument, "zero-sized buffer is rejected");

    auto upload = device.createBuffer(
        {.size = kSize, .usage = rhi::BufferUsage::CopySrc, .memory = rhi::MemoryLocation::CpuToGpu});
    auto gpu = device.createBuffer({.size = kSize,
                                    .usage = rhi::BufferUsage::CopySrc | rhi::BufferUsage::CopyDst,
                                    .memory = rhi::MemoryLocation::GpuOnly});
    auto readback = device.createBuffer(
        {.size = kSize, .usage = rhi::BufferUsage::CopyDst, .memory = rhi::MemoryLocation::GpuToCpu});
    if (!upload || !gpu || !readback) {
        check(false, "buffers created");
        return;
    }
    check(!(*gpu)->map({}), "GpuOnly buffer cannot be mapped");

    auto mapped = (*upload)->map({});
    check(mapped && mapped->size() == kSize, "upload buffer maps to its full size");
    if (!mapped) {
        return;
    }
    check(!(*upload)->map({}), "a mapped buffer cannot be mapped twice");
    for (std::size_t i = 0; i < mapped->size(); ++i) {
        (*mapped)[i] = static_cast<std::byte>((i * 31 + 7) & 0xFF);
    }
    (*upload)->unmap();

    auto cmd = device.createCommandBuffer({.queue = rhi::QueueType::Transfer});
    if (!cmd) {
        check(false, cmd.error().message.c_str());
        return;
    }
    check(!(*cmd)->beginCopy(), "beginCopy before begin() returns null");
    check(static_cast<bool>((*cmd)->begin()), "command buffer begin");
    {
        rhi::CopyEncoderRef copy = (*cmd)->beginCopy();
        check(copy && !(*cmd)->beginCopy(), "only one encoder at a time");
        copy->copyBuffer({.src = *upload, .dst = *gpu});
        copy->end();
    }
    (*cmd)->barrier(
        {.buffers = {{.buffer = *gpu, .before = rhi::ResourceState::CopyDst, .after = rhi::ResourceState::CopySrc}}});
    {
        rhi::CopyEncoderRef copy = (*cmd)->beginCopy();
        copy->copyBuffer({.src = *gpu, .dst = *readback});
        copy->end();
    }
    check(static_cast<bool>((*cmd)->end()), "command buffer end");

    auto fence = device.createFence({});
    if (!fence) {
        return;
    }
    rhi::QueueRef queue = device.getQueue(rhi::QueueType::Transfer);
    check(static_cast<bool>(queue->submit({.command_buffers = {*cmd}, .fence = *fence})), "copy submitted");
    check(static_cast<bool>((*fence)->wait(std::chrono::seconds(5))), "copy finished");

    auto result = (*readback)->map({});
    auto source = (*upload)->map({});
    bool same = result && source && std::equal(result->begin(), result->end(), source->begin(), source->end());
    check(same, "GPU round trip: read-back bytes match the upload");
    (*readback)->unmap();
    (*upload)->unmap();

    check((*cmd)->begin() && (*cmd)->end(), "command buffer can be re-recorded");
}

#if defined(MANGO_RHI_VULKAN) || defined(MANGO_RHI_METAL)

void testCompute(rhi::IDevice& device) {
    constexpr std::uint32_t kCount = 1024;
    auto shader = device.createShader(
        {.stage = rhi::ShaderStage::Compute, .spirv = {std::begin(fill_comp_spv), std::end(fill_comp_spv)}});
    auto group_layout =
        device.createBindGroupLayout({.entries = {{0, rhi::BindingType::StorageBuffer, rhi::ShaderStage::Compute}}});
    if (!shader || !group_layout) {
        check(false, "compute shader and bind group layout created");
        return;
    }
    auto layout = device.createPipelineLayout({.bind_group_layouts = {*group_layout}});
    check(!device.createComputePipeline({.layout = layout ? *layout : nullptr}), "compute pipeline needs a shader");
    auto pipeline = device.createComputePipeline({.layout = layout ? *layout : nullptr, .compute_shader = *shader});
    check(pipeline && (*pipeline)->getKind() == rhi::PipelineKind::Compute, "compute pipeline created");

    auto output = device.createBuffer({.size = kCount * sizeof(std::uint32_t),
                                       .usage = rhi::BufferUsage::Storage,
                                       .memory = rhi::MemoryLocation::GpuToCpu});
    if (!pipeline || !output) {
        check(false, "compute pipeline and storage buffer created");
        return;
    }
    auto group = device.createBindGroup({.layout = *group_layout, .entries = {{.binding = 0, .buffer = *output}}});
    check(!device.createBindGroup({.layout = *group_layout, .entries = {{.binding = 0}}}),
          "bind group entry without its buffer is rejected");
    check(!device.createBindGroup({.layout = *group_layout, .entries = {{.binding = 5, .buffer = *output}}}),
          "bind group entry for a missing binding is rejected");

    auto cmd = device.createCommandBuffer({.queue = rhi::QueueType::Compute});
    auto fence = device.createFence({});
    if (!group || !cmd || !fence || !(*cmd)->begin()) {
        check(false, "compute bind group and command buffer ready");
        return;
    }
    {
        rhi::ComputeEncoderRef compute = (*cmd)->beginCompute();
        check(compute && !(*cmd)->beginCompute(), "compute encoder begins, only one at a time");
        compute->setPipeline(*pipeline);
        compute->setBindGroup({.index = 0, .bind_group = *group});
        compute->dispatch({.x = kCount / 64});
        compute->end();
    }
    check((*cmd)->end() &&
              device.getQueue(rhi::QueueType::Compute)->submit({.command_buffers = {*cmd}, .fence = *fence}),
          "dispatch submitted");
    check(static_cast<bool>((*fence)->wait(std::chrono::seconds(5))), "dispatch finished");

    auto mapped = (*output)->map({});
    bool correct = static_cast<bool>(mapped);
    for (std::uint32_t i = 0; correct && i < kCount; ++i) {
        std::uint32_t value = 0;
        std::memcpy(&value, mapped->data() + i * sizeof(value), sizeof(value));
        correct = value == i * i + 7;
    }
    check(correct, "compute read-back: values[i] == i * i + 7");
    (*output)->unmap();
}
#endif

void testLifetime(rhi::IDevice& device) {
    auto gate = device.createSemaphore({.kind = rhi::SemaphoreKind::Timeline});
    auto blocked = device.createCommandBuffer({});
    auto fence = device.createFence({});
    if (!gate || !blocked || !fence || !(*blocked)->begin() || !(*blocked)->end()) {
        check(false, "lifetime test objects ready");
        return;
    }
    rhi::QueueRef queue = device.getQueue(rhi::QueueType::Graphics);
    check(
        static_cast<bool>(queue->submit(
            {.command_buffers = {*blocked}, .waits = {{*gate, 1, rhi::PipelineStage::AllCommands}}, .fence = *fence})),
        "gated submit");
    check(!(*blocked)->begin(), "begin() fails while the submission is pending");
    check(!queue->submit({.command_buffers = {*blocked}}), "a pending command buffer cannot be resubmitted");

    rhi::SemaphoreRef gate_ref = *gate;
    blocked = rhi::Error{};
    gate = rhi::Error{};
    check(!gate_ref->signal(0), "signal() needs a value above the current one");
    check(static_cast<bool>(gate_ref->signal(1)) && gate_ref->getValue() == 1, "host signal opens the gate");
    gate_ref.reset();
    check(static_cast<bool>((*fence)->wait(std::chrono::seconds(5))), "gated submission finished");
}

void testTextureRoundTrip(rhi::IDevice& device, const rhi::Capabilities& caps) {
    constexpr std::uint32_t kSize = 64;
    constexpr std::uint32_t kRegion = 32;
    constexpr rhi::Offset3D kOrigin{16, 8, 0};
    constexpr std::uint32_t kTexel = 4;

    auto bad = device.createTexture({.format = rhi::Format::Undefined, .usage = rhi::TextureUsage::Sampled});
    check(!bad && bad.error().code == rhi::ErrorCode::InvalidArgument, "texture without a format is rejected");

    auto source = device.createTexture({
        .format = rhi::Format::RGBA8Unorm,
        .extent = {kSize, kSize},
        .usage = rhi::TextureUsage::CopyDst | rhi::TextureUsage::CopySrc | rhi::TextureUsage::Sampled,
        .label = "source",
    });
    auto region = device.createTexture({
        .format = rhi::Format::RGBA8Unorm,
        .extent = {kRegion, kRegion},
        .usage = rhi::TextureUsage::CopyDst | rhi::TextureUsage::CopySrc,
    });
    auto upload = device.createBuffer(
        {.size = kSize * kSize * kTexel, .usage = rhi::BufferUsage::CopySrc, .memory = rhi::MemoryLocation::CpuToGpu});
    auto readback = device.createBuffer({.size = kRegion * kRegion * kTexel,
                                         .usage = rhi::BufferUsage::CopyDst,
                                         .memory = rhi::MemoryLocation::GpuToCpu});
    if (!source || !region || !upload || !readback) {
        check(false, "textures and buffers created");
        return;
    }

    auto view = device.createTextureView({.texture = *source});
    check(view && (*view)->getTexture() == *source, "texture view created and retains its texture");
    check(!device.createTextureView({}), "texture view without a texture is rejected");

    auto depth = device.createTexture(
        {.format = rhi::Format::D32Float, .extent = {kSize, kSize}, .usage = rhi::TextureUsage::DepthStencil});
    check(depth && device.createTextureView({.texture = *depth, .aspect = rhi::TextureAspect::DepthOnly}),
          "depth texture and depth view created");

    check(static_cast<bool>(device.createSampler({})), "default sampler created");
    check(static_cast<bool>(device.createSampler({.compare = rhi::CompareOp::LessOrEqual})), "comparison sampler");
    if (caps.features.sampler_anisotropy) {
        check(static_cast<bool>(device.createSampler({.max_anisotropy = caps.limits.max_sampler_anisotropy})),
              "anisotropic sampler created");
    }

    auto mapped = (*upload)->map({});
    if (!mapped) {
        return;
    }
    for (std::uint32_t y = 0; y < kSize; ++y) {
        for (std::uint32_t x = 0; x < kSize; ++x) {
            std::byte* texel = mapped->data() + (y * kSize + x) * kTexel;
            texel[0] = static_cast<std::byte>(x);
            texel[1] = static_cast<std::byte>(y);
            texel[2] = static_cast<std::byte>(x ^ y);
            texel[3] = std::byte{255};
        }
    }
    (*upload)->unmap();

    auto cmd = device.createCommandBuffer({});
    if (!cmd || !(*cmd)->begin()) {
        check(false, "texture command buffer begins");
        return;
    }
    (*cmd)->barrier(
        {.textures = {
             {.texture = *source, .before = rhi::ResourceState::Undefined, .after = rhi::ResourceState::CopyDst}}});
    {
        rhi::CopyEncoderRef copy = (*cmd)->beginCopy();
        copy->copyBufferTexture(
            {.buffer = {.buffer = *upload}, .texture = {.texture = *source}, .extent = {kSize, kSize}});
        copy->end();
    }
    (*cmd)->barrier(
        {.textures = {
             {.texture = *source, .before = rhi::ResourceState::CopyDst, .after = rhi::ResourceState::CopySrc},
             {.texture = *region, .before = rhi::ResourceState::Undefined, .after = rhi::ResourceState::CopyDst},
         }});
    {
        rhi::CopyEncoderRef copy = (*cmd)->beginCopy();
        copy->copyTexture({.src = {.texture = *source, .origin = kOrigin},
                           .dst = {.texture = *region},
                           .extent = {kRegion, kRegion}});
        copy->end();
    }
    (*cmd)->barrier(
        {.textures = {
             {.texture = *region, .before = rhi::ResourceState::CopyDst, .after = rhi::ResourceState::CopySrc}}});
    {
        rhi::CopyEncoderRef copy = (*cmd)->beginCopy();
        copy->copyBufferTexture({.direction = rhi::CopyDirection::TextureToBuffer,
                                 .buffer = {.buffer = *readback},
                                 .texture = {.texture = *region},
                                 .extent = {kRegion, kRegion}});
        copy->end();
    }
    check(static_cast<bool>((*cmd)->end()), "texture copies recorded");

    auto fence = device.createFence({});
    if (!fence) {
        return;
    }
    check(device.getQueue(rhi::QueueType::Graphics)->submit({.command_buffers = {*cmd}, .fence = *fence}) &&
              (*fence)->wait(std::chrono::seconds(5)),
          "texture copies finished");

    auto result = (*readback)->map({});
    bool same = static_cast<bool>(result);
    for (std::uint32_t y = 0; same && y < kRegion; ++y) {
        for (std::uint32_t x = 0; same && x < kRegion; ++x) {
            const std::byte* texel = result->data() + (y * kRegion + x) * kTexel;
            const std::uint32_t sx = x + kOrigin.x;
            const std::uint32_t sy = y + kOrigin.y;
            same = texel[0] == static_cast<std::byte>(sx) && texel[1] == static_cast<std::byte>(sy) &&
                   texel[2] == static_cast<std::byte>(sx ^ sy) && texel[3] == std::byte{255};
        }
    }
    check(same, "texture round trip: read-back region matches the source texels");
    (*readback)->unmap();
}

void testOffscreenClear(rhi::IDevice& device) {
    constexpr std::uint32_t kSize = 32;
    constexpr std::array<float, 4> kClear = {0.25f, 0.5f, 0.75f, 1.0f};
    constexpr float kDepth = 0.5f;

    auto color = device.createTexture({.format = rhi::Format::RGBA8Unorm,
                                       .extent = {kSize, kSize},
                                       .usage = rhi::TextureUsage::RenderTarget | rhi::TextureUsage::CopySrc});
    auto depth = device.createTexture({.format = rhi::Format::D32Float,
                                       .extent = {kSize, kSize},
                                       .usage = rhi::TextureUsage::DepthStencil | rhi::TextureUsage::CopySrc});
    auto color_readback = device.createBuffer(
        {.size = kSize * kSize * 4, .usage = rhi::BufferUsage::CopyDst, .memory = rhi::MemoryLocation::GpuToCpu});
    auto depth_readback = device.createBuffer({.size = kSize * kSize * sizeof(float),
                                               .usage = rhi::BufferUsage::CopyDst,
                                               .memory = rhi::MemoryLocation::GpuToCpu});
    if (!color || !depth || !color_readback || !depth_readback) {
        check(false, "offscreen textures and buffers created");
        return;
    }
    auto color_view = device.createTextureView({.texture = *color});
    auto depth_view = device.createTextureView({.texture = *depth});

    check(!device.createRenderPass({}), "render pass without attachments is rejected");
    auto pass = device.createRenderPass({
        .color_attachments = {{.format = rhi::Format::RGBA8Unorm}},
        .depth_stencil_attachment = rhi::AttachmentDesc{.format = rhi::Format::D32Float},
    });
    check(static_cast<bool>(pass), "render pass created");
    if (!pass || !color_view || !depth_view) {
        return;
    }
    check(!device.createRenderTarget({.render_pass = *pass, .extent = {kSize, kSize}}),
          "render target with missing attachments is rejected");
    auto target = device.createRenderTarget({.render_pass = *pass,
                                             .color_attachments = {*color_view},
                                             .depth_stencil_attachment = *depth_view,
                                             .extent = {kSize, kSize}});
    check(static_cast<bool>(target), "render target created");
    if (!target) {
        return;
    }

    auto cmd = device.createCommandBuffer({});
    if (!cmd || !(*cmd)->begin()) {
        check(false, "render command buffer begins");
        return;
    }
    (*cmd)->barrier(
        {.textures = {
             {.texture = *color, .before = rhi::ResourceState::Undefined, .after = rhi::ResourceState::RenderTarget},
             {.texture = *depth,
              .before = rhi::ResourceState::Undefined,
              .after = rhi::ResourceState::DepthStencilWrite},
         }});
    {
        rhi::RenderEncoderRef render = (*cmd)->beginRender({.render_pass = *pass,
                                                            .target = *target,
                                                            .clear_colors = {kClear},
                                                            .clear_depth_stencil = {.depth = kDepth}});
        check(render && !(*cmd)->beginCopy(), "render encoder open; no other encoder meanwhile");
        if (render) {
            render->end();
        }
    }
    (*cmd)->barrier(
        {.textures = {
             {.texture = *color, .before = rhi::ResourceState::RenderTarget, .after = rhi::ResourceState::CopySrc},
             {.texture = *depth, .before = rhi::ResourceState::DepthStencilWrite, .after = rhi::ResourceState::CopySrc},
         }});
    {
        rhi::CopyEncoderRef copy = (*cmd)->beginCopy();
        copy->copyBufferTexture({.direction = rhi::CopyDirection::TextureToBuffer,
                                 .buffer = {.buffer = *color_readback},
                                 .texture = {.texture = *color},
                                 .extent = {kSize, kSize}});
        copy->copyBufferTexture({.direction = rhi::CopyDirection::TextureToBuffer,
                                 .buffer = {.buffer = *depth_readback},
                                 .texture = {.texture = *depth},
                                 .extent = {kSize, kSize}});
        copy->end();
    }
    check(static_cast<bool>((*cmd)->end()), "render pass recorded");

    auto fence = device.createFence({});
    if (!fence) {
        return;
    }
    check(device.getQueue(rhi::QueueType::Graphics)->submit({.command_buffers = {*cmd}, .fence = *fence}) &&
              (*fence)->wait(std::chrono::seconds(5)),
          "render pass finished");

    auto pixels = (*color_readback)->map({});
    bool color_ok = static_cast<bool>(pixels);
    for (std::size_t i = 0; color_ok && i < pixels->size(); ++i) {
        const int expected = static_cast<int>(kClear[i % 4] * 255.0f + 0.5f);
        color_ok = std::abs(static_cast<int>((*pixels)[i]) - expected) <= 1;
    }
    check(color_ok, "colour attachment cleared to the clear colour");
    (*color_readback)->unmap();

    auto depths = (*depth_readback)->map({});
    bool depth_ok = static_cast<bool>(depths);
    for (std::size_t i = 0; depth_ok && i < kSize * kSize; ++i) {
        float value = 0.0f;
        std::memcpy(&value, depths->data() + i * sizeof(float), sizeof(float));
        depth_ok = value == kDepth;
    }
    check(depth_ok, "depth attachment cleared to the clear depth");
    (*depth_readback)->unmap();
}

#if defined(MANGO_RHI_VULKAN) || defined(MANGO_RHI_METAL)

void testScissorAndStencil(rhi::IDevice& device) {
    constexpr std::uint32_t kSize = 32;
    auto color = device.createTexture({.format = rhi::Format::RGBA8Unorm,
                                       .extent = {kSize, kSize},
                                       .usage = rhi::TextureUsage::RenderTarget | rhi::TextureUsage::CopySrc});
    auto depth_stencil = device.createTexture(
        {.format = rhi::Format::D32FloatS8Uint, .extent = {kSize, kSize}, .usage = rhi::TextureUsage::DepthStencil});
    auto readback = device.createBuffer(
        {.size = kSize * kSize * 4, .usage = rhi::BufferUsage::CopyDst, .memory = rhi::MemoryLocation::GpuToCpu});
    auto vertex = device.createShader(
        {.stage = rhi::ShaderStage::Vertex, .spirv = {std::begin(fullscreen_vert_spv), std::end(fullscreen_vert_spv)}});
    auto fragment = device.createShader(
        {.stage = rhi::ShaderStage::Fragment, .spirv = {std::begin(red_frag_spv), std::end(red_frag_spv)}});
    auto layout = device.createPipelineLayout({});
    auto pass = device.createRenderPass({
        .color_attachments = {{.format = rhi::Format::RGBA8Unorm}},
        .depth_stencil_attachment = rhi::AttachmentDesc{.format = rhi::Format::D32FloatS8Uint,
                                                        .store_op = rhi::StoreOp::DontCare,
                                                        .stencil_load_op = rhi::LoadOp::Clear},
    });
    if (!color || !depth_stencil || !readback || !vertex || !fragment || !layout || !pass) {
        check(false, "scissor/stencil resources created");
        return;
    }
    auto color_view = device.createTextureView({.texture = *color});
    auto depth_view = device.createTextureView({.texture = *depth_stencil});
    if (!color_view || !depth_view) {
        check(false, "scissor/stencil views created");
        return;
    }
    auto target = device.createRenderTarget({.render_pass = *pass,
                                             .color_attachments = {*color_view},
                                             .depth_stencil_attachment = *depth_view,
                                             .extent = {kSize, kSize}});
    auto pipeline = [&](rhi::DepthStencilState state, rhi::ColorWriteMask mask) {
        return device.createGraphicsPipeline({.layout = *layout,
                                              .vertex_shader = *vertex,
                                              .fragment_shader = *fragment,
                                              .depth_stencil = state,
                                              .blend = {{.write_mask = mask}},
                                              .render_pass = *pass});
    };
    auto plain = pipeline({}, rhi::ColorWriteMask::All);
    auto write_stencil = pipeline(
        {.stencil_test = true, .stencil = {.compare = rhi::CompareOp::Always, .pass_op = rhi::StencilOp::Replace}},
        rhi::ColorWriteMask::None);
    auto test_stencil =
        pipeline({.stencil_test = true, .stencil = {.compare = rhi::CompareOp::Equal}}, rhi::ColorWriteMask::All);
    check(plain && write_stencil && test_stencil, "pipelines with stencil state created");
    auto cmd = device.createCommandBuffer({});
    if (!target || !plain || !write_stencil || !test_stencil || !cmd || !(*cmd)->begin()) {
        check(false, "scissor/stencil command buffer begins");
        return;
    }
    (*cmd)->barrier(
        {.textures = {
             {.texture = *color, .before = rhi::ResourceState::Undefined, .after = rhi::ResourceState::RenderTarget},
             {.texture = *depth_stencil,
              .before = rhi::ResourceState::Undefined,
              .after = rhi::ResourceState::DepthStencilWrite},
         }});
    {
        rhi::RenderEncoderRef render =
            (*cmd)->beginRender({.render_pass = *pass, .target = *target, .clear_colors = {{0.0f, 0.0f, 0.0f, 1.0f}}});

        render->setPipeline(*plain);
        render->setScissor({0, 0, 4, 4});
        render->draw({.vertex_count = 3});

        render->setScissor({-10, -10, 5, 5});
        render->draw({.vertex_count = 3});

        render->setPipeline(*write_stencil);
        render->setStencilReference(1);
        render->setScissor({8, 8, 16, 16});
        render->draw({.vertex_count = 3});

        render->setPipeline(*test_stencil);
        render->setScissor({0, 0, 1000, 1000});
        render->draw({.vertex_count = 3});
        render->end();
    }
    (*cmd)->barrier(
        {.textures = {
             {.texture = *color, .before = rhi::ResourceState::RenderTarget, .after = rhi::ResourceState::CopySrc}}});
    {
        rhi::CopyEncoderRef copy = (*cmd)->beginCopy();
        copy->copyBufferTexture({.direction = rhi::CopyDirection::TextureToBuffer,
                                 .buffer = {.buffer = *readback},
                                 .texture = {.texture = *color},
                                 .extent = {kSize, kSize}});
        copy->end();
    }
    auto fence = device.createFence({});
    if (!(*cmd)->end() || !fence ||
        !device.getQueue(rhi::QueueType::Graphics)->submit({.command_buffers = {*cmd}, .fence = *fence}) ||
        !(*fence)->wait(std::chrono::seconds(5))) {
        check(false, "scissor/stencil frame finished");
        return;
    }
    auto pixels = (*readback)->map({});
    if (!pixels) {
        check(false, "scissor/stencil read-back maps");
        return;
    }
    auto red = [&](std::uint32_t x, std::uint32_t y) { return (*pixels)[(y * kSize + x) * 4] == std::byte{255}; };
    bool scissor_ok = true;
    bool stencil_ok = true;
    for (std::uint32_t y = 0; y < kSize; ++y) {
        for (std::uint32_t x = 0; x < kSize; ++x) {
            const bool in_corner = x < 4 && y < 4;
            const bool in_centre = x >= 8 && x < 24 && y >= 8 && y < 24;
            if (in_corner != red(x, y) && !in_centre) {
                scissor_ok = false;
            }
            if (in_centre != red(x, y) && !in_corner) {
                stencil_ok = false;
            }
        }
    }
    (*readback)->unmap();
    check(scissor_ok, "scissor rectangles restrict draws (and are clamped to the target)");
    check(stencil_ok, "stencil writes and tests mask draws");
}
#endif

void testFactory() {
    rhi::RHIFactory factory;
    rhi::BackendList available = factory.availableBackends();
    std::printf("Available backends:");
    for (rhi::Backend backend : available) {
        std::printf(" %s", std::string(rhi::toString(backend)).c_str());
    }
    std::printf("\n");
    check(std::ranges::all_of(available,
                              [](rhi::Backend backend) {
                                  return std::ranges::find(rhi::compiledBackends(), backend) !=
                                         rhi::compiledBackends().end();
                              }),
          "RHIFactory lists only compiled-in backends as available");
#if defined(MANGO_RHI_VULKAN)
    const bool has_vulkan_loader = vk::detail::DynamicLoader().success();
    check((std::ranges::find(available, rhi::Backend::Vulkan) != available.end()) == has_vulkan_loader,
          "RHIFactory lists Vulkan exactly when its loader is found");
#endif

    auto gl = factory.createInstance(rhi::Backend::GL, {});
    check(!gl && gl.error().code == rhi::ErrorCode::BackendUnavailable,
          "RHIFactory rejects a backend that is not compiled in");
}

void testBackend(rhi::Backend backend) {
    const std::string name(rhi::toString(backend));
    std::printf("== %s ==\n", name.c_str());
    rhi::RHIFactory factory;

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
                if (message.message.starts_with("using ")) {
                    layer_in_use = message.message;
                }
                std::printf("  [debug] %.*s\n", static_cast<int>(message.message.size()), message.message.data());
            },
    };

    {
        auto instance = factory.createInstance(backend, desc);
        check(static_cast<bool>(instance),
              instance ? "instance created with validation" : instance.error().message.c_str());
        if (!instance) {
            return;
        }

        rhi::AdapterList adapters = (*instance)->enumerateAdapters();
        check(!adapters.empty(), "at least one adapter");
        check((*instance)->enumerateAdapters().size() == adapters.size(), "adapters are enumerated once and cached");

        for (const rhi::AdapterRef& adapter : adapters) {
            rhi::Capabilities caps = adapter->getCapabilities();
            std::printf("Adapter: %s (%s), %s %u.%u.%u, vendor 0x%04x device 0x%04x\n", caps.name.c_str(),
                        adapterTypeName(caps.type), name.c_str(), caps.api_version_major, caps.api_version_minor,
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
            testSync(**device);
            devices.push_back(*device);
            testBufferRoundTrip(**device);
            testTextureRoundTrip(**device, caps);
            testLifetime(**device);
            testOffscreenClear(**device);
#if defined(MANGO_RHI_VULKAN) || defined(MANGO_RHI_METAL)
            testScissorAndStencil(**device);
            testCompute(**device);
#endif
        }

        instance = rhi::Error{};
        check(adapters.empty() || !adapters[0]->getCapabilities().name.empty(), "adapters outlive their instance");

        adapters.clear();
        rhi::QueueRef queue = devices.empty() ? nullptr : devices[0]->getQueue(rhi::QueueType::Graphics);
        devices.clear();
        check(devices.empty() && (adapters.empty() || queue), "queues outlive their device");
    }

    if (backend == rhi::Backend::Metal) {
        check(layer_in_use == "using Metal API validation", "Metal API validation is active");
        check(problems == 0, "no validation warnings or errors");
        return;
    }
    if (backend != rhi::Backend::Vulkan) {
        check(problems == 0, "no validation warnings or errors");
        return;
    }
#if defined(MANGO_RHI_VULKAN) && defined(MANGO_VULKAN_VALIDATION)

    const std::string expected_layer = "using VK_LAYER_KHRONOS_validation " +
                                       std::to_string(VK_API_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE)) + "." +
                                       std::to_string(VK_API_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE)) + "." +
                                       std::to_string(VK_API_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE));
    check(layer_in_use == expected_layer, "the pinned validation layer built from source is the one in use");
    check(problems == 0, "no validation warnings or errors");
#else
    std::printf("Validation layer not built (MANGO_VULKAN_VALIDATION=OFF); %d warning(s)/error(s)\n", problems);
#endif
}

} // namespace

int main() {
    testResult();
    testTypes();
    testInterfaces();
    testBackends();
    testFactory();
    for (rhi::Backend backend : rhi::RHIFactory().availableBackends()) {
        testBackend(backend);
    }

    if (g_failures != 0) {
        std::printf("%d check(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }
    std::printf("All checks passed\n");
    return EXIT_SUCCESS;
}
