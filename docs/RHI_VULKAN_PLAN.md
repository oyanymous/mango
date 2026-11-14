# Vulkan RHI implementation plan

Implements the RHI designed in `docs/diagrams/` with only the Vulkan backend. Metal and GL stay
design-only; the public interfaces are written so they can be added later without API changes.

Each step ends in something that builds and can be checked on its own. Work stops after every
step for review before the next one starts.

## Decisions (apply to all steps)

| Topic | Choice | Why |
|---|---|---|
| Location | New static library `RHI` in `src/rhi/` (public interfaces); Vulkan sources in `src/rhi/vulkan/`, compiled into the same target when `MANGO_RHI_VULKAN=ON` | Matches the "compiled registry" in `RHI_CORE.mmd`; follows the per-OS-subdir pattern of `src/platform`. |
| Layering | `RHI → Core`. RHI does **not** depend on `Platform`; windows are passed in as a native handle inside `SurfaceDesc` | Keeps the GPU layer independent of the windowing layer. |
| Header hygiene | No `vulkan.h` in public RHI headers; Vulkan types only in `src/rhi/vulkan/` | Same goal as the pimpl rule for AppKit types. |
| Language | C++20 | Designated initializers for `*Desc` structs, `std::span`. |
| `Result<T>` | Small project-owned `Result<T>` / `Result<void>` holding a value or an `Error {code, message}` | `std::expected` needs C++23; can be swapped later. |
| `*Ref` types | `using BufferRef = std::shared_ptr<IBuffer>;` etc. | Diagrams use "retains" relationships everywhere. |
| Vulkan SDK | LunarG Vulkan SDK via `find_package(Vulkan REQUIRED COMPONENTS glslc)`; MoltenVK on macOS | Ships headers, loader, validation layers and `glslc`. |
| API level | Vulkan 1.2 + timeline semaphores; classic `VkRenderPass`/`VkFramebuffer` as in `RHI_RENDER_TARGETS.mmd` | Works on MoltenVK; matches the diagrams. |
| Memory | Vulkan Memory Allocator (VMA) via `FetchContent` | Avoids hand-written allocators. Diagrams' `VkDeviceMemory` members become `VmaAllocation` (diagrams updated in the last step). |
| Shaders | GLSL compiled to SPIR-V at build time with `glslc`; `ShaderDesc` takes SPIR-V bytes | No runtime compiler dependency. |
| Debugging | Validation layers + debug messenger on in Debug builds; any validation error fails the smoke tests | Gives each step an objective "clean" check. |
| Verification | `samples/rhi_smoke` (headless, registered with CTest) and `samples/rhi_triangle` (windowed) | Repo has no tests yet; these become them. |

## Steps

### 1. Build setup and empty RHI library
- Add a `windows` configure/build preset (MSVC, Debug, `build/windows/`) next to `macos`; set C++20.
- Add `MANGO_RHI_VULKAN` option (default ON), `find_package(Vulkan)`, fetch VMA.
- Create `src/rhi/` target `RHI` with `Result.h`, `Backend.h` (`enum class Backend { Vulkan, Metal, GL }`)
  and common value types (`Extent2D/3D`, `Format`, `QueueType`, …).
- Add `samples/rhi_smoke` that links `RHI` and prints the Vulkan header version; register with CTest.
- **Verify:** both presets configure and build; `main` still opens its window; `ctest` passes.

### 2. Public RHI interfaces (headers only)
- One header per interface from the diagrams: `IInstance`, `IAdapter`, `IDevice`, `IQueue`, `ISurface`,
  `ISwapchain`, `IBuffer`, `ITexture`, `ITextureView`, `ISampler`, `IShader`, `IPipelineLayout`,
  `IPipeline`, `IBindGroupLayout`, `IBindGroup`, `IRenderPass`, `IRenderTarget`, `ICommandBuffer`,
  `IRenderEncoder`, `IComputeEncoder`, `ICopyEncoder`, `IFence`, `ISemaphore`, `IDeviceFactory`, `RHIFactory`.
- All `*Desc` structs, enums (formats, usages, load/store ops, …) and `*Ref` aliases.
- **Verify:** headers compile in a test TU; signatures match the diagrams 1:1 (I'll list any deviations).

### 3. Factory, instance and adapters
- `RHIFactory::availableBackends/createInstance`, `VulkanDeviceFactory`, `VulkanInstance`
  (validation layers, debug messenger, required surface extensions), `VulkanAdapter` with `Capabilities`.
- **Verify:** `rhi_smoke` lists the GPUs with name, type and limits; zero validation messages.

### 4. Device and queues
- `VulkanAdapter::createDevice`: queue-family selection (graphics/compute/transfer), feature enablement,
  VMA allocator. `VulkanDevice::getQueue`, `VulkanQueue` (submit stubbed).
- **Verify:** device created and destroyed cleanly; validation clean on shutdown (no leaked objects).

### 5. Synchronization
- `VulkanFence` (wait / isSignaled / reset), `VulkanSemaphore` (binary and timeline), `VulkanQueue::submit`
  with wait/signal semaphores and fence.
- **Verify:** smoke test does an empty submit, waits on the fence, and checks a timeline value advances.

### 6. Buffers, command buffers and copy encoder
- `VulkanBuffer` (VMA, `map`/`unmap`), `VulkanCommandBuffer` (begin/end, resource retention),
  `VulkanCopyEncoder::copyBuffer`, `barrier()` for buffers.
- **Verify:** upload data → GPU copy → read back → bytes match. First real GPU round trip.

### 7. Textures, views, samplers
- `VulkanTexture`, `VulkanTextureView`, `VulkanSampler`; image layout tracking and texture barriers;
  `copyBufferTexture` and `copyTexture`.
- **Verify:** upload an image, copy texture→texture→buffer, read back and compare.

### 8. Render passes, render targets and the render encoder (offscreen)
- `VulkanRenderPass`, `VulkanRenderTarget` (framebuffer), `ICommandBuffer::beginRender` with clear values,
  `VulkanRenderEncoder` shell (begin/end only).
- **Verify:** clear an offscreen texture to a known color, read back, check pixels.

### 9. Surface, swapchain and presentation
- Platform: expose a native window handle (`HWND`/`HINSTANCE` on Windows, `CAMetalLayer` on macOS) and a
  per-frame / resize callback from the run loop.
- `VulkanSurface`, `VulkanSwapchain` (acquire, backbuffer textures, `resize`, out-of-date recreation),
  `VulkanQueue::present`, frames in flight.
- Add `samples/rhi_triangle` (just clears for now).
- **Verify:** window shows an animated clear color; resizing and minimizing work without validation errors.

### 10. Shaders and graphics pipeline — first triangle
- CMake rule compiling `.vert/.frag` → `.spv`; `VulkanShader`, `VulkanPipelineLayout`,
  `createGraphicsPipeline`, render encoder `setPipeline`, `setVertexBuffers`, `setIndexBuffer`, `draw`, `drawIndexed`.
- **Verify:** colored triangle in the window, from a vertex buffer, plus an indexed quad.

### 11. Bind groups
- `VulkanBindGroupLayout`, `VulkanBindGroup` (descriptor pool), `setBindGroup`.
- **Verify:** rotating textured quad driven by a uniform buffer + texture + sampler.

### 12. Compute
- `createComputePipeline`, `VulkanComputeEncoder` (`setPipeline`, `setBindGroup`, `dispatch`).
- **Verify:** compute shader fills a storage buffer; smoke test reads back and checks values.

### 13. Lifetime hardening and docs
- Deferred destruction of resources still in flight, `m_retained_resources` release on fence completion,
  clean shutdown order, error paths return `Result` errors instead of asserting.
- Update the `.mmd` diagrams where the implementation diverged (e.g. VMA members), `CLAUDE.md`
  (new targets, presets, Vulkan SDK requirement).
- **Verify:** all samples run validation-clean, including window close mid-frame; diagrams render.

## Before step 1
- The `rhi` branch has uncommitted diagram edits; they should be committed (or stashed) first so the
  implementation commits stay separate.
- Install the LunarG Vulkan SDK (not found on this machine: `VULKAN_SDK` is unset) and make sure `cmake`
  is on `PATH`.
