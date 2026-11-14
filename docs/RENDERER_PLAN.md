# Scene-graph renderer implementation plan

Adds a retained-mode 2D renderer on top of the RHI (`src/rhi/I*.h`). The application describes what is on
screen as a **scene graph** of nodes; the renderer turns the graph into RHI work and does as little of it as
possible: nothing at all when nothing changed, and only the changed part of the window when something did.

The renderer never talks to Vulkan or Metal. It only uses the public RHI, so both backends run the same
renderer code and are checked by the same tests.

## Status

All thirteen steps are done (November 2026), implemented in one go and verified on macOS with Metal (`ctest`, and
`scenegraph_demo` / `main` in a window). The Vulkan code paths compile but could not be run on this Mac (no MoltenVK);
`scenegraph_tests` and `rhi_smoke` run them on any machine with a Vulkan driver. Deviations from the plan:

- **Materials:** flat colour and vertex colour are one material (`Material { color, texture, smooth }`, the colour
  multiplies the vertex colours), so there are two fragment shaders (colour, texture) instead of three material types.
  A material colour change updates the node's slot only; texture or filtering changes rebuild batches.
- **Step 8:** `FrameMode::OnDemand` and `Application::requestFrame()` (the default stays `Continuous`, so
  `rhi_triangle` is unchanged); `Window::devicePixelRatio()` was added for the renderer.
- **Step 9:** besides `setScissor`, the encoder gained `setViewport` and `setStencilReference`. Metal API validation
  asserts on redundant state calls and on a scissor replaced before any draw used it, so `MetalRenderEncoder` now
  filters redundant state (`MetalRenderState`) and applies viewport and scissor lazily at the next draw.
- **Step 12:** stencil state is `DepthStencilState::stencil_test` + one `StencilFaceState` for both faces; the canvas
  uses `D32FloatS8Uint`, falling back to `D24UnormS8Uint`.
- **Layers** are drawn into textures that grow in 64-pixel steps, and are always fully re-rendered when their content
  changes (no damage tracking inside a layer).
- **Step 13:** the optional `RenderNode` escape hatch was not added.

## Workflow

Same as the RHI plans: each step ends in something that builds and can be checked on its own.

1. I implement the step, build the `macos` preset, run `ctest --preset macos` (and the windowed demo when the
   step touches it, with `--backend vulkan` and `--backend metal`), and report what I ran and what I could not check.
2. Work stops. You verify.
3. On your OK I commit (one Conventional Commit per step, e.g. `feat(scenegraph): add geometry nodes`), and
   push the branch, then start the next step.

## Goals and non-goals

- **Goals:** a node tree (transform, clip, opacity, geometry nodes); dirty tracking so an unchanged frame costs
  no GPU work and almost no CPU work; partial redraw of damaged regions; batching of compatible draws; uploads
  of only the data that changed; identical output on Vulkan and Metal.
- **Non-goals for this plan:** text rendering, 3D (depth-sorted meshes, cameras, lighting), multithreaded
  rendering, animations/timelines (the app changes nodes; the renderer only reacts). Each can be a later plan.

## How it works (overview)

```
 app code ──mutates──▶ scene graph ──dirty bits──▶ Renderer::render()
                                                     1. nothing dirty? → return (no acquire, submit, present)
                                                     2. update pass: walk dirty subtrees only, recompute
                                                        world matrix / opacity / clip, collect damage rects
                                                     3. upload: changed vertices, indices, per-node data
                                                     4. batches: rebuilt only if structure/material changed
                                                     5. draw into the persistent canvas, scissored to damage
                                                     6. composite canvas → swapchain image, present
```

### Nodes

All nodes derive from `sg::Node`, which owns its children (`std::unique_ptr`), knows its parent, and holds
dirty bits. Node kinds:

| Node | Holds | Effect on its subtree |
|---|---|---|
| `Node` | children | Plain grouping. |
| `RootNode` | the tree, a back pointer to the `Renderer` | Receives dirty notifications; one per renderer. |
| `TransformNode` | a 2D affine `Transform2D` | Children's world matrix = parent world × this matrix. |
| `OpacityNode` | `float opacity` (0–1) | Multiplies into children's inherited opacity; opacity 0 culls the subtree. |
| `ClipNode` | a clip rectangle in local coordinates | Children are clipped to it (intersected with any outer clip). |
| `GeometryNode` | a `Geometry` + a `Material` | Draws something. The only node that produces draw calls. |
| `RectangleNode`, `ImageNode` | convenience `GeometryNode`s | Solid-colour rect; textured rect. |
| `LayerNode` (step 11) | an offscreen texture | Renders its subtree once into a texture and reuses it until the subtree changes. |

`Geometry` is CPU-side vertex and index data plus its vertex layout and topology (triangles, triangle strip,
lines). `Material` describes how to shade it: a material *type* (flat colour, vertex colour, texture) that
selects shaders and a pipeline, plus per-instance values (colour, texture view, sampler). Geometry and
material are owned by the node (`std::shared_ptr` so several nodes can share one, e.g. a common texture).

### Dirty tracking

Every setter marks the node dirty with what changed:

```cpp
enum class DirtyFlags : std::uint32_t {
    None = 0,
    Matrix = 1u << 0,      // TransformNode matrix changed
    Opacity = 1u << 1,     // OpacityNode value changed
    Clip = 1u << 2,        // ClipNode rect changed
    Geometry = 1u << 3,    // GeometryNode vertices/indices changed
    Material = 1u << 4,    // material values changed (colour, texture)
    MaterialType = 1u << 5,// material type changed → different pipeline → batches rebuilt
    NodeAdded = 1u << 6,   // structure changed → batches rebuilt
    NodeRemoved = 1u << 7,
    Subtree = 1u << 8,     // set on ancestors: "something below me is dirty"
};
```

`markDirty(flags)` sets the flags on the node and walks up setting `Subtree` on ancestors, stopping at the
first ancestor that already has it (so marking is O(depth) once, then O(1)). When it reaches the root, the
root tells its renderer, which requests a frame from the application (step 8). The update pass descends only
into nodes with `Subtree` or their own flags set and clears them as it goes; a clean subtree is skipped whole.

Inherited state (world matrix, opacity, clip) is cached per node. A `Matrix`, `Opacity` or `Clip` change on a
node invalidates the cached state of its whole subtree, but nothing outside it.

### Damage and partial redraw

The window is drawn into a **persistent canvas texture** that survives between frames; swapchain images do not
(there are 2–3 of them, rotated, with undefined contents on Metal). For every dirty geometry node the update
pass adds its *previous* device-space bounds and its *new* bounds to the frame's damage region; removed
nodes add their last bounds; a resize damages everything. The damage region is a small list of rectangles,
merged when they overlap and collapsed into one when there are too many (default 8) or they cover more
than half the window (then a full redraw is cheaper).

Each damage rectangle is redrawn by: setting the scissor to it, clearing it (a background quad), and drawing
only the batches whose bounds intersect it, in tree order. The canvas is then composited onto the acquired
swapchain image with one full-screen triangle and presented.

### Batching and GPU data

- **One draw per batch, not per node.** Geometry of nodes that share a material type and render state is
  placed in shared vertex/index pools; consecutive nodes in tree (paint) order that share a pipeline, bind
  group and clip become one batch drawn with a single `drawIndexed`.
- **Per-node data without push constants.** The RHI has no push constants or dynamic offsets, so each
  geometry node owns a slot in a GPU-only storage buffer holding `{world matrix, opacity, colour, clip id}`.
  Vertices carry their node's slot index as an attribute, so a whole batch is drawn at once and a node that
  only moved updates 48 bytes, not its vertices.
- **Uploads only for changes.** Changed slots and changed geometry ranges are written to a per-frame
  `CpuToGpu` staging ring and copied into the GPU-only pools with `ICopyEncoder::copyBuffer` at the start
  of the frame's command buffer, followed by a `CopyDst → ShaderRead/VertexBuffer` barrier. Pools grow by
  reallocating (a full re-upload) and free ranges are reused.
- **Batches are rebuilt only on `NodeAdded`, `NodeRemoved`, `MaterialType`, or a geometry that changed size.**
  Moving, fading or recolouring a node keeps every batch as is.

## Decisions (apply to all steps)

| Topic | Choice | Why |
|---|---|---|
| Location | New static library target `SceneGraph` in `src/scenegraph/`, namespace `sg`, links `RHI` and `Core` publicly. Exports its own directory like the other modules | Keeps the RHI generic; dependencies become `UI → SceneGraph → RHI → Core` and `UI → Platform → Core`. |
| Platform independence | The renderer does not depend on `Platform`. It is handed an `rhi::DeviceRef` and an `rhi::SurfaceRef` (the caller creates the surface from `Window::nativeWindow()`), plus the window's pixel size and device pixel ratio | Same rule as the RHI: windows enter as native handles. Tests can run it headless on an offscreen target. |
| Header names | Unique across modules: `SceneNode.h`, `TransformNode.h`, `GeometryNode.h`, `Geometry.h`, `Material.h`, `Renderer.h`, `Transform2D.h`, `RectF.h` (not `Rect.h`, which `src/ui` owns) | Headers are included bare. |
| Dimension | 2D only: `Transform2D` is a 3×2 affine matrix, coordinates are `float` logical pixels, origin top-left, +Y down | Matches the UI layer (`Point`, `Rect`, `Polygon`). Vulkan and Metal already agree on clip space (Metal shaders are Y-flipped by the backend), so one projection works on both. |
| Ownership | Parents own children (`std::unique_ptr<Node>`); geometry, materials and textures are `std::shared_ptr`. GPU resources freed by the renderer when a node is destroyed (the RHI keeps in-flight work alive on its own) | Clear tree ownership; sharing where it is natural. |
| Paint order | Tree order, depth first, children after parents (painter's algorithm); no depth buffer | Correct for 2D with transparency; batching must preserve it. |
| Clipping | Axis-aligned clips (the common case) become scissor rectangles. Clips under a rotation are drawn conservatively with the scissor of their bounding box until step 12 adds stencil clipping | The RHI has no stencil ops yet; scissoring is free. |
| Blending | Premultiplied alpha everywhere (`One, OneMinusSrcAlpha`); opacity multiplies all four channels | Correct compositing of nested opacity; one blend state for all materials. |
| Shaders | GLSL in `src/scenegraph/shaders/`, compiled with `mango_add_shaders` (flat colour, vertex colour, texture, composite) | Same SPIR-V for both backends. |
| Pipelines | Cached by `(material type, render pass, topology)`; created on first use | A handful of pipelines for the whole app. |
| Frames in flight | 2, each with its own command buffer, staging ring, fence and semaphores, as in `rhi_triangle` | The CPU prepares frame N+1 while the GPU draws frame N. |
| Threading | Single-threaded, called from the application's frame callback | The RHI is not thread-safe. |
| Instrumentation | `Renderer::lastFrameStats()`: frame skipped, nodes visited, batches, draw calls, bytes uploaded, damage area | Lets tests assert that "nothing changed" really costs nothing. |
| Verification | `samples/scenegraph_tests` (headless, in CTest, runs per available backend: CPU-only tree tests plus offscreen renders read back and compared pixel by pixel) and `samples/scenegraph_demo [--backend vulkan\|metal] [--frames N]` (windowed, not in CTest) | Same split as `rhi_smoke` / `rhi_triangle`. |

## Steps

### 1. Library skeleton, math types and the node tree
- `src/scenegraph/` target `SceneGraph`; `Transform2D` (multiply, invert, map point/rect), `RectF`
  (intersect, unite, empty), `Node` with child management (`appendChild`, `insertChildBefore`, `removeChild`,
  returning ownership on removal), `RootNode`, `TransformNode`, `OpacityNode`, `ClipNode`.
- `samples/scenegraph_tests` with CPU-only checks; registered with CTest.
- **Verify:** tests for tree operations and math pass; `windows` preset still configures.

### 2. Dirty tracking
- `DirtyFlags`, `markDirty` with `Subtree` propagation and the early stop, root → renderer notification hook
  (a callback for now), the update traversal that skips clean subtrees and caches world matrix, opacity and clip.
- **Verify:** tests: changing one leaf visits only its ancestors' path; a second update with no change visits
  only the root; moving a `TransformNode` recomputes exactly its subtree.

### 3. Geometry and materials (CPU side)
- `Geometry` (vertex layout, vertex/index bytes, topology, `markVerticesDirty`), helpers for rectangles,
  textured rectangles, polylines with width and convex polygons (from `ui::Polygon`-like point lists);
  `Material` types `FlatColorMaterial`, `VertexColorMaterial`, `TextureMaterial`; `GeometryNode`,
  `RectangleNode`, `ImageNode`; local and device-space bounds.
- **Verify:** tests for generated vertex data and bounds under transforms.

### 4. Renderer setup and the first full redraw (offscreen)
- `Renderer` created from a device and either a surface or an offscreen texture; the canvas texture,
  render pass (load existing contents), composite pass, shaders, pipeline cache, per-frame resources.
- Full redraw every frame, one draw per node, no batching yet: proves geometry, materials, transforms and
  premultiplied opacity end to end.
- **Verify:** offscreen renders of a small scene (nested transforms, opacity, a texture) read back and
  match expected pixels on Vulkan and Metal; validation clean.

### 5. Windowed demo and the swapchain path
- `samples/scenegraph_demo`: window, surface, swapchain, resize handling (recreate the canvas, damage all),
  minimized windows skipped, a scene with a few hundred nodes, some animated by the demo's frame callback.
- **Verify:** demo runs on both backends, resize and minimize work, `--frames N` exits clean.

### 6. GPU pools, per-node storage slots and incremental uploads
- Vertex/index pools with range allocation, the per-node storage buffer, the staging ring, copy + barrier at
  the start of the frame; only dirty nodes write anything.
- **Verify:** tests on stats: moving one node uploads one slot (48 bytes) and no vertices; changing its
  geometry uploads only its range; pixels still match step 4's references.

### 7. Batching
- Batch building in paint order (same pipeline, bind group, clip → one batch), rebuilt only on structural
  or material-type changes; one `drawIndexed` per batch.
- **Verify:** tests: 1 000 same-coloured rects draw in 1 call; interleaving a texture in the middle splits
  correctly (order preserved: pixel test with overlapping shapes); moving a node does not rebuild batches.

### 8. Skip unchanged frames (platform)
- `Application::requestFrame()`: the event loop calls the frame callback only when a frame was requested
  (and sleeps in `nextEventMatchingMask` / `GetMessage` otherwise), instead of spinning whenever a callback is
  set. The renderer requests a frame when its root becomes dirty and on resize; `render()` returns without
  acquiring, submitting or presenting when nothing is dirty.
- **Verify:** stats test: a clean frame reports `skipped` with zero draws and uploads; the demo with
  animation paused uses ~0 % CPU and GPU (Activity Monitor); `rhi_triangle` (which requests every frame)
  is unaffected.

### 9. RHI: scissor rectangles
- `IRenderEncoder::setScissor(Rect2D)` (and `setViewport`) for Vulkan (`vkCmdSetScissor`, already dynamic) and
  Metal (`setScissorRect:`), clamped to the target; `rhi_smoke` check that draws a full-screen quad with a
  scissor and reads back pixels inside and outside it. Diagrams (`RHI_COMMANDS.mmd`) updated.
- **Verify:** smoke checks pass on both backends; validation clean.

### 10. Clip nodes and partial redraw
- Clip nodes as scissor rects (intersected down the tree, part of the batch key); damage collection
  (old ∪ new bounds), merging and the full-redraw fallback, scissored clear + redraw of intersecting batches only,
  composite of the canvas.
- **Verify:** tests: moving a 10×10 rect in a 1 000×1 000 scene damages ≤ 2 rects and draws only the batches
  under them; the read-back image equals a full redraw of the same scene (pixel exact) after a sequence of
  random edits; demo shows no trails or stale pixels while resizing and animating.

### 11. Layer nodes (cached subtrees)
- `LayerNode` renders its subtree into its own texture and draws it as one textured quad; the texture is
  re-rendered only when something in the subtree is dirty, so moving or fading a complex but static subtree
  costs one quad. Resized when its bounds change.
- **Verify:** a layer with 500 children moved across the window: stats show 1 draw for it per frame and no
  re-render of its subtree; pixels match the un-layered scene within rounding.

### 12. Stencil clipping for rotated clips (RHI + renderer)
- `DepthStencilState` stencil ops in the RHI (both backends, smoke-checked), a `D32FloatS8Uint` canvas
  attachment, `ClipNode` under a non-axis-aligned transform writes its shape to the stencil buffer and its
  subtree tests against it. Nested clips increment the reference.
- **Verify:** pixel tests for a rotated clip and a nested rotated clip on both backends.

### 13. Hooking the UI layer up and docs
- `Widget` owns a node subtree (a `TransformNode` at its position); the top-level widget's window owns a
  `Renderer`; `main` draws a small scene through widgets. Optional `RenderNode` escape hatch that lets custom
  code record its own RHI commands inside the graph.
- Doxygen for every public header; `docs/Mainpage.md` gets a "Using the renderer" section; a Mermaid class
  diagram of the scene graph in `docs/diagrams/SCENEGRAPH.mmd`; `CLAUDE.md` updated (new target, dependency
  flow, tests). Keep the Doxygen warnings log empty.
- **Verify:** `main` shows the widget scene on both backends; all CTest suites pass; diagrams render.

## Risks and open questions

- **Canvas + composite costs one extra full-screen pass per changed frame.** Cheaper than full redraws once
  scenes are non-trivial, and it removes per-swapchain-image damage history. If profiling shows it matters,
  the alternative is "buffer age" damage tracking per swapchain image, which needs the RHI to guarantee that
  backbuffers keep their contents (Metal does not by default).
- **Storage buffer in the vertex stage** must be supported by every target GPU (it is on Apple silicon via
  Metal and on Vulkan 1.2 desktop GPUs). If a GL backend is added later, it may need a uniform-buffer fallback.
- **Large geometry changes every frame** (e.g. a plotted line that changes completely) defeat the incremental
  path; such nodes can be flagged `dynamic` to stream their vertices each frame instead of using pool ranges.
- **Text** will need a glyph atlas and its own material; it fits the design (a `TextNode` is a `GeometryNode`
  of textured quads) but is left for a later plan.
