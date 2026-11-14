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
| `src/scenegraph` | `SceneGraph` | The scene graph (nodes, geometry, materials) and sg::Renderer, which draws it with the RHI. |
| `src/rhi` | `RHI` | The GPU abstraction in namespace @ref rhi, with the Vulkan backend in @ref rhi::vulkan and the Metal backend in @ref rhi::metal. |

Dependencies flow `UI → Platform → Core`, `UI → SceneGraph → RHI → Core`. The RHI does not depend on the platform layer: windows are
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

## Using the renderer

The scene graph library (`src/scenegraph`, namespace @ref sg) draws retained 2D content through the RHI. Build a tree
of nodes under an sg::RootNode, hand it to an sg::Renderer, and call render() when the application asks for a frame:

@code
auto renderer = sg::Renderer::create({.device = device, .surface = surface, .size = {800, 600},
                                      .device_pixel_ratio = window.devicePixelRatio(),
                                      .request_frame = [] { Application::requestFrame(); }});
sg::RootNode root;
auto* moving = root.emplaceChild<sg::TransformNode>();
moving->emplaceChild<sg::RectangleNode>(sg::RectF{0, 0, 100, 50}, sg::ColorF{1, 0, 0, 1});
(*renderer)->setRootNode(&root);
Application::setFrameCallback([&] { (*renderer)->render(); }, FrameMode::OnDemand);
moving->setMatrix(sg::Transform2D::translation(20, 0)); // redraws only around the rectangle
@endcode

- Nodes: sg::TransformNode, sg::OpacityNode, sg::ClipNode (rotated clips use the stencil buffer), sg::GeometryNode
  (sg::RectangleNode, sg::ImageNode) and sg::LayerNode (caches its subtree in a texture).
- Setters mark nodes dirty; the renderer's update pass (sg::UpdatePass) visits only dirty paths. A frame where nothing
  changed does no GPU work, and with FrameMode::OnDemand the event loop sleeps.
- Changed pixels (the damage) are redrawn into a persistent canvas, which is then copied to the window. Consecutive
  nodes with the same pipeline, texture and clip are drawn with one call; moving a node uploads 48 bytes.
- Widget builds on it: every widget is a small subtree (position, background, clip), and each window has a renderer.

## Further reading

- `docs/RHI_VULKAN_PLAN.md` — the step-by-step plan the Vulkan backend is built by.
- `docs/RENDERER_PLAN.md` — the plan the scene graph renderer was built by.
- `docs/diagrams/` — UML class diagrams of the RHI design (all three backends).
