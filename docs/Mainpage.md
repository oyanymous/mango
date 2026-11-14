# Mango {#mainpage}

Mango is an early-stage cross-platform renderer. It has two halves: a small windowing/UI layer, and a
**rendering hardware interface (RHI)** that hides the GPU API (Vulkan and Metal today; GL is designed but not
implemented).

## Modules

| Module | Target | What it holds |
|---|---|---|
| `src/core` | `Core` (header-only) | Interfaces shared across layers, e.g. IWidget. |
| `src/platform` | `Platform` | The native application and windows: Application, Window, with per-OS code in `darwin/` and `windows/`. |
| `src/ui` | `UI` | Widget and geometry/value types (Point, Size, Rect, Line, Polygon, Color). |
| `src/rhi` | `RHI` | The GPU abstraction in namespace @ref rhi, with the Vulkan backend in @ref rhi::vulkan and the Metal backend in @ref rhi::metal. |

Dependencies flow `UI → Platform → Core` and `RHI → Core`. The RHI does not depend on the platform layer: windows are
handed to it as native handles (rhi::SurfaceDesc).

## Using the RHI

Everything starts from rhi::RHIFactory:

@code
rhi::RHIFactory factory;
auto instance = factory.createInstance(rhi::Backend::Vulkan, {.enable_validation = true});
if (!instance) {
    std::puts(instance.error().message.c_str());
    return;
}
for (const rhi::AdapterRef& adapter : (*instance)->enumerateAdapters()) {
    rhi::Capabilities caps = adapter->getCapabilities();
    // pick an adapter, then adapter->createDevice({...})
}
@endcode

The object model, from top to bottom:

- rhi::IInstance — connection to one backend; enumerates rhi::IAdapter (physical GPUs) and creates rhi::ISurface.
- rhi::IDevice — the logical GPU, created by an adapter. It owns the rhi::IQueue objects and creates every resource:
  buffers, textures, samplers, shaders, pipelines, bind groups, render passes and targets, command buffers,
  swapchains, fences and semaphores.
- rhi::ICommandBuffer — records work through one scoped encoder at a time (rhi::IRenderEncoder,
  rhi::IComputeEncoder, rhi::ICopyEncoder), separated by rhi::ICommandBuffer::barrier calls.
- rhi::IQueue — submits command buffers and presents swapchain images.

Conventions used across the API:

- Every object is created from a plain `*Desc` struct and handed back as a reference-counted `*Ref`
  (`std::shared_ptr`); holding a Ref keeps the object alive. Encoders are the exception: they are scoped to one
  recording pass and owned by `std::unique_ptr`.
- Operations that can fail return rhi::Result, which holds either the value or an rhi::Error.
- Resource transitions are explicit: rhi::BarrierBatch moves resources between rhi::ResourceState values.

## Further reading

- `docs/RHI_VULKAN_PLAN.md` — the step-by-step plan the Vulkan backend is built by.
- `docs/diagrams/` — UML class diagrams of the RHI design (all three backends).
