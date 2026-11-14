# Metal RHI implementation plan

Adds a Metal backend behind the existing public RHI interfaces (`src/rhi/I*.h`). The public API does not
change; every Metal-specific decision stays in `src/rhi/metal/`. Vulkan stays the reference backend: each
step mirrors a step of `docs/RHI_VULKAN_PLAN.md` and is checked with the same smoke checks.

## Workflow

Each step ends in something that builds and can be checked on its own:

1. I implement the step, build the `macos` preset, run `ctest --preset macos` (and the windowed sample when
   the step touches it), and report what I ran and what I could not check.
2. Work stops. You verify.
3. On your OK I commit (one Conventional Commit per step, e.g. `feat(rhi): add Metal device and queues`)
   and push the branch, then start the next step.

## Prerequisites

- Steps 1–8 only need what Vulkan has today (through step 9, presentation) and can start now.
- Step 9 needs Vulkan step 10 (the glslang GLSL → SPIR-V build rule and the pipeline interfaces in use).
- Step 10 needs Vulkan step 11 (bind groups); step 11 needs Vulkan step 12 (compute).

## Decisions (apply to all steps)

| Topic | Choice | Why |
|---|---|---|
| Location | `src/rhi/metal/`, compiled into the existing `RHI` target when `MANGO_RHI_METAL=ON` (default ON on Apple, forced OFF elsewhere). Backend headers included bare (`"MetalDevice.h"`), private to RHI | Same layout as `src/rhi/vulkan/`. |
| Language | Objective-C++ (`.mm`) talking to the Metal API directly, not metal-cpp | Matches `src/platform/darwin`; no extra dependency. |
| Header hygiene | Public RHI headers never include Metal/ObjC headers. Backend headers that hold `id<MTL…>` members are included only from `.mm` files. `Metal`, `QuartzCore` and `Foundation` frameworks linked `PRIVATE` | Same rule as Vulkan; the existing header-check target keeps proving it. |
| Memory management | ARC (`-fobjc-arc`) on `src/rhi/metal/*.mm` only; the rest of the project stays manual reference counting | Every backend object holds several `id<MTL…>`s; ARC removes a whole class of leaks/over-releases. Per-file, so platform code is untouched. |
| Minimum OS | macOS 13 (Metal 3), set as `CMAKE_OSX_DEPLOYMENT_TARGET` | Covers every Apple-silicon Mac and the APIs used below (`MTLSharedEvent`, GPU families). |
| Shaders | `ShaderDesc` stays SPIR-V. The Metal backend converts SPIR-V → MSL with **SPIRV-Cross**, fetched at the same `vulkan-sdk-*` tag as Vulkan-Headers (`cmake/MetalDependencies.cmake`), then compiles with `newLibraryWithSource:` | One shader source for both backends, no API change, no full Xcode needed (Command Line Tools have no `metal` compiler). Precompiled `.metallib` can come later. |
| Binding model | Flat remap: the pipeline layout assigns each `(set, binding)` a per-stage Metal buffer/texture/sampler index, handed to SPIRV-Cross; vertex buffers take buffer indices from 30 downwards | Simple and validates easily. Argument buffers are an optimisation for later. |
| Queues | One `MTLCommandQueue` per `QueueType` | Metal has no queue families; separate queues keep `getQueue` semantics. |
| Command buffers | `ICommandBuffer::begin()` creates a fresh `MTLCommandBuffer` (Metal ones are one-shot) | Keeps the reusable RHI command-buffer object. |
| Barriers | Resources use Metal's default hazard tracking, so `barrier()` records no GPU work; it only checks and updates the tracked `ResourceState` (debug builds report an illegal `before` state) | Same RHI usage as Vulkan, catches bugs on both. |
| Sync | `ISemaphore` → `MTLSharedEvent` (timeline = value; binary = internal counter). `IFence` → command-buffer completion handler + condition variable | Works on macOS 13; no busy waits. |
| Buffer memory | `MemoryLocation` GPU-only → `MTLStorageModePrivate`; CPU-visible → `MTLStorageModeShared` | Unified memory on Apple silicon; avoids `Managed` + `didModifyRange` bookkeeping. |
| Formats | `D24UnormS8Uint` reported unsupported when the device lacks it (all Apple silicon); everything else maps 1:1 | Matches the note on `Format`. |
| Debugging | `enable_validation` turns on the Metal API validation layer (`MTL_DEBUG_LAYER=1`, plus shader validation) before the first Metal call; command-buffer errors (`MTLCommandBufferErrorOption.EncoderExecutionStatus`) are routed to `debug_callback`. A validation failure aborts, so CTest fails | Gives each step the same "clean" check Vulkan has. |
| Verification | `rhi_smoke` runs its backend checks once per available backend; `rhi_triangle` takes `--backend metal\|vulkan` | Both backends checked by the same code. |

## Steps

### 1. Build setup and backend-agnostic smoke checks
- `MANGO_RHI_METAL` option, `src/rhi/metal/` with an empty `MetalDeviceFactory`, ARC flags, frameworks,
  deployment target. `compiledBackends()` lists Metal; `RHIFactory` registers it.
- Restructure `rhi_smoke` so the instance/device/resource checks take a `Backend` and run for every
  available backend (Vulkan checks unchanged). Vulkan without a driver on macOS (no MoltenVK yet) is
  skipped, not failed.
- **Verify:** `macos` configures and builds with Metal ON and OFF; `ctest` passes; `windows` preset unaffected.

### 2. Factory, instance and adapters
- `MetalDeviceFactory::isAvailable`, `MetalInstance` (validation setup, debug callback), `MetalAdapter`
  over `MTLCopyAllDevices()`: name, type (integrated/discrete/external from `hasUnifiedMemory`,
  `isRemovable`), Metal GPU family as API version, features and limits.
- **Verify:** `rhi_smoke` lists the Mac's GPU(s) under Metal with sensible limits; validation clean.

### 3. Device, queues and synchronization
- `MetalAdapter::createDevice`, `MetalDevice::getQueue`, `MetalQueue::submit` (waits/signals encoded with
  `encodeWaitForEvent`/`encodeSignalEvent`, fence signaled from the completion handler), `MetalFence`,
  `MetalSemaphore` (binary and timeline).
- **Verify:** the Vulkan step-5 checks pass on Metal: empty submit, fence wait/reset, timeline value advances.

### 4. Buffers, command buffers and copy encoder
- `MetalBuffer` (storage modes, `map`/`unmap`), `MetalCommandBuffer` (begin/end, resource retention,
  state-tracking `barrier()`), `MetalCopyEncoder::copyBuffer` (blit encoder).
- **Verify:** upload → GPU copy → read back → bytes match, on Metal.

### 5. Textures, views, samplers
- `MetalTexture`, `MetalTextureView` (`newTextureViewWithPixelFormat:`), `MetalSampler`
  (`MTLSamplerState`); format table; `copyBufferTexture` and `copyTexture`.
- **Verify:** the Vulkan step-7 texture round trip passes on Metal.

### 6. Render passes, render targets and the render encoder (offscreen)
- `MetalRenderPass` (formats + load/store ops), `MetalRenderTarget` (attachment views),
  `beginRender` builds an `MTLRenderPassDescriptor` with clear values; `MetalRenderEncoder` shell.
- **Verify:** clear an offscreen texture to a known colour, read back, check pixels.

### 7. macOS window: CAMetalLayer, resize and frame callbacks (platform)
- `src/platform/darwin/Window.mm`: layer-backed content view with a `CAMetalLayer`, `nativeWindow()`
  returns it, `drawableSize` follows backing scale, resize callback and a per-frame callback from the run
  loop (display link). Removes the current "shortcut" there; the same handle serves Vulkan via MoltenVK later.
- **Verify:** `main` and `rhi_triangle` still open a window; resize callback fires with pixel sizes.

### 8. Surface, swapchain and presentation
- `MetalSurface` (wraps the layer), `MetalSwapchain` (layer format/size/`displaySyncEnabled` from the present
  mode, `maximumDrawableCount` for frames in flight; `acquire` = `nextDrawable`, the backbuffer `ITexture`
  wraps the drawable's texture), `MetalQueue::present` (`presentDrawable:` after the waited semaphores), `resize`.
- `rhi_triangle --backend metal`.
- **Verify:** window shows the animated clear colour on Metal; resize and minimize work; validation clean.

### 9. Shaders and graphics pipeline — first triangle
- Fetch SPIRV-Cross; `MetalShader` (SPIR-V → MSL, kept until pipeline creation so bindings can be remapped),
  `MetalPipelineLayout` (binding remap tables), `createGraphicsPipeline` (`MTLRenderPipelineState` +
  `MTLDepthStencilState` + raster state), encoder `setPipeline`, `setVertexBuffers`, `setIndexBuffer`,
  `draw`, `drawIndexed`.
- **Verify:** the same triangle and indexed quad as Vulkan, from the same SPIR-V.

### 10. Bind groups
- `MetalBindGroupLayout`, `MetalBindGroup` (resolved resource lists), `setBindGroup` binding to the
  remapped per-stage indices.
- **Verify:** the rotating textured quad (uniform buffer + texture + sampler) matches Vulkan.

### 11. Compute
- `createComputePipeline` (`MTLComputePipelineState`, threadgroup size from the shader's local size),
  `MetalComputeEncoder` (`setPipeline`, `setBindGroup`, `dispatch`).
- **Verify:** the compute storage-buffer check passes on Metal.

### 12. Lifetime hardening and docs
- Resources retained until their command buffer completes, clean shutdown order, error paths return
  `Result` errors (shader conversion/compile errors carry the compiler log).
- Update `Backend.h` docs, the `.mmd` diagrams where Metal diverged (SPIRV-Cross, flat binding remap,
  `MTLSharedEvent`), `CLAUDE.md` and `docs/Mainpage.md`; keep the Doxygen warnings log empty.
- **Verify:** both backends pass `rhi_smoke`; `rhi_triangle` runs validation-clean on Metal, including
  closing the window mid-frame; diagrams render.
