# docs/diagrams

Mermaid diagrams documenting the proposed rendering hardware interface (RHI): an abstraction over Metal, Vulkan and GL that the renderer is written against. Use the `mermaid-uml` skill when creating or editing these files.

## Files

- `RHI_DESIGN.mmd`: subsystem dependency map and entry point.
- `RHI_CORE.mmd`: factories, instances, adapters, and device APIs.
- `RHI_RESOURCES.mmd`: buffers, textures, views, and samplers.
- `RHI_PIPELINES_BINDINGS.mmd`: shaders, pipelines, and resource bindings.
- `RHI_RENDER_TARGETS.mmd`: render passes and targets.
- `RHI_COMMANDS.mmd`: command recording and scoped encoders.
- `RHI_PRESENTATION.mmd`: surfaces and swapchains.
- `RHI_SUBMISSION_SYNC.mmd`: queues and synchronization.

Keep detailed API relationships in their owning subsystem file, and update `RHI_DESIGN.mmd` when subsystem dependencies change. Before changing a type's API, find every file that repeats it (e.g. `rg -n ITextureView docs/diagrams`).

## Style

- Four-space indentation; keep `direction LR`. `flowchart LR` for the overview, `classDiagram` for subsystem details. Name new files `RHI_<SUBSYSTEM>.mmd`.
- PascalCase types, `I` prefix for interfaces, `Metal` / `Vulkan` / `GL` prefixes for backend classes. camelCase functions and parameters, `m_snake_case` members (e.g. `getDesc()`, `m_command_buffer`). Method parameters list types only: `createBuffer(BufferDesc) Result`.
- Mark interfaces `<<interface>>`, public members `+`, private state `-`, comments `%%`. An interface owned by another file and repeated only for context is marked `<<external>>` and must match its owning file.
- A backend class implementing an interface is a realization, `IBuffer <|.. MetalBuffer`; the solid `<|--` is only for interface-to-interface inheritance. `..>` means creates/uses, `-->` means a retained reference.
- Keep the header comments that mark the design as proposed and native types as conceptual.
- Keep any `%% siren-layout {...}` line (Siren Studio's saved node positions); rename or remove its keys when renaming or deleting a class.

## Validation

Render every changed diagram and check labels, relationships and readability, and that the Metal, Vulkan and GL implementations of a changed interface stay consistent:

```bash
npx -y @mermaid-js/mermaid-cli -i RHI_CORE.mmd -o RHI_CORE.svg
```

Do not commit generated SVGs. For pull requests, list the affected diagrams and include rendered screenshots when layout or relationships change materially.
