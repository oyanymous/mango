# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & run

CMake (≥ 3.24), C++20, one preset per host OS (Debug, output in `build/<preset>/`):

- `macos`: Apple Clang.
- `windows`: MSVC (`cl`, x64) + Ninja. Needs Visual Studio 2022 17.10+ or Build Tools with the "Desktop development with C++" workload (the root `CMakeLists.txt` rejects older MSVC). `cl` must be on the environment: run from a Developer PowerShell / x64 Native Tools prompt, or use CLion with a "Visual Studio" toolchain (or Visual Studio itself). MinGW is not supported for the Vulkan backend's validation layer.

```bash
cmake --preset macos
cmake --build --preset macos
ctest --preset macos
./build/macos/src/main
```

The Vulkan RHI backend (`MANGO_RHI_VULKAN`, default ON) needs **no Vulkan SDK**: `cmake/VulkanDependencies.cmake` fetches Vulkan-Headers, volk and VMA at pinned versions (keep Vulkan-Headers and volk on the same `vulkan-sdk-*` tag). volk loads the GPU driver's Vulkan loader at runtime, so nothing links against it; their implementations are compiled once in `src/rhi/vulkan/*Implementation.cpp`. Do not add `find_package(Vulkan)` or other SDK lookups. Configure with `-DMANGO_RHI_VULKAN=OFF` to build without the backend. `MANGO_VULKAN_VALIDATION` (default ON, but OFF with MinGW: CLion's bundled MinGW cannot link the layer's SPIRV-Tools dependency, which forces `-static`) builds the Khronos validation layer from source at the same tag (`cmake/VulkanValidationLayers.cmake`, an ExternalProject; needs Python 3; the first build takes a long time, later builds skip it). Instances created with `enable_validation` set `VK_LAYER_PATH` to that build, so an installed SDK's layer is ignored unless `VK_LAYER_PATH`/`VK_ADD_LAYER_PATH` is already set.

Tests: `samples/rhi_smoke` is a headless check program registered with CTest; it exits non-zero on any failed check.

## Formatting & git hooks

Code style is defined by `.clang-format` (4-space indent, attached braces, `Type* ptr`, 120 columns). Format with `clang-format -i <files>`.

Configuring CMake sets `core.hooksPath` to `.githooks/` (or run `git config core.hooksPath .githooks` by hand):

- `commit-msg` rejects messages that are not Conventional Commits (see Conventions).
- `pre-commit` runs clang-format on the staged C/C++/ObjC++ files: a file is reformatted and re-staged automatically, unless it also has unstaged changes, in which case the commit is rejected. clang-format comes from `$CLANG_FORMAT`, `PATH`, a CLion install, or Homebrew LLVM. If none is found, any commit that includes C++ files is rejected.

Hook scripts must keep LF line endings (enforced by `.gitattributes`) and the executable bit. The top-level `build/` directory (outside `build/macos/`) holds stale output from an earlier non-preset configure (e.g. an old `Renderer` target) — ignore it.

## Architecture

Early-stage macOS renderer. `src/main.cpp` creates an `Application` and a top-level `Widget`, then calls `app.run()` (blocks in the Cocoa run loop).

Four library targets, with dependencies flowing `UI → Platform → Core` and `RHI → Core`:

- **`src/core`** — header-only `INTERFACE` target `Core`. Holds abstract interfaces shared across layers (e.g. `IWidget`), so `src/platform` can refer to widgets without depending on `src/ui`.
- **`src/platform`** — static library target `Platform` (links `Core` and Cocoa publicly).
  - `Application` is a thin public facade over the `PrivateApplication` singleton, which owns the list of top-level `IWidget*`s and the native app state. Constructing `Application` initializes `[NSApplication sharedApplication]`, so it must come before any `Window` or `Widget`.
  - Classes holding native state (`PrivateApplication`, `Window`) use the **pimpl idiom** (`struct Impl; std::unique_ptr<Impl> m_impl;`) so their headers stay plain C++ and no Objective-C/AppKit types leak into C++ translation units.
  - Platform-specific implementations live in per-OS subdirectories; `src/platform` are Objective-C++ files that define `Impl` with AppKit objects (`NSApplication`, `NSWindow`, delegate classes). New OS backends would follow the same pattern alongside `darwin/`.
  - The project enables the `OBJCXX` language in the root `src/CMakeLists.txt`.
- **`src/ui`** — static library target `UI` (links `Platform` and `Core`). `Widget` implements `IWidget`; a parentless `Widget` registers itself via `Application::addTopLevelWidget`. Also holds geometry/value types (`Point`, `Size`, `Rect`, `Line`, `Polygon`, `Color`).
- **`src/rhi`** — static library target `RHI` (links `Core`): the GPU abstraction from `docs/diagrams/`, in `namespace rhi`, being built step by step per `docs/RHI_VULKAN_PLAN.md`. Backend code lives in `src/rhi/vulkan/` and is compiled in when `MANGO_RHI_VULKAN` is ON; Vulkan and VMA are linked `PRIVATE`, so public RHI headers must never include Vulkan headers. RHI does not depend on `Platform`.
- ARC is not enabled (no `-fobjc-arc`), so Objective-C objects in `.mm` files are under manual reference counting.

### Include style

Each library exports **its own directory** as its include path (`Core` via `INTERFACE`, `Platform` via `PUBLIC`), so headers are included **bare**, without a module prefix, both inside and across modules: `#include "IWidget.h"`, `#include "Window.h"`, `#include "Application.h"`. A `"platform/..."` or `"core/..."` path will not resolve from another module. When adding a new library, export its own directory the same way. Header names must therefore be unique across modules.

`src/main.cpp` is the exception: it uses `"platform/Application.h"` / `"ui/Widget.h"`, which resolve only because it sits at the repo root and quoted includes search the including file's directory first.

## Design docs

`docs/diagrams/` holds Mermaid UML diagrams of the planned RHI (GPU abstraction over Metal, Vulkan and GL). Its own `docs/diagrams/CLAUDE.md` has the diagram conventions and validation steps.

## Documentation

API docs are generated by Doxygen (pinned 1.18.0, fetched by `cmake/Docs.cmake`): configure with `-DMANGO_BUILD_DOCS=ON`, build the `docs` target, open `build/<preset>/docs/html/index.html`. `docs/Mainpage.md` is the front page. Keep `build/<preset>/docs/warnings.log` empty.

Comment style (Doxygen with `///`, `JAVADOC_AUTOBRIEF`, as in LLVM):

- Every file starts with `/// @file` and a one-sentence summary after `#pragma once` / before the first include.
- Document every public class, function, enum and struct in its **header** with `///` directly above it. The first sentence (ending in `.`) is the brief; a blank `///` line starts the details.
- Use `@param name`, `@return`, `@note`, `@warning`, `@see`, `@code`/`@endcode`. Skip `@param`/`@return` only when the brief already says everything.
- Struct fields and enumerators get a trailing `///<` comment (or a `///` line above when it does not fit).
- `.cpp`/`.mm` files: `/// @file` summary, `///` on file-local helpers, and plain `//` comments that explain *why* inside function bodies. Do not repeat the header's API docs.
- Say what a reader cannot see from the signature: ownership and lifetime, threading, when it fails, units, defaults.

## Conventions

- Member fields use the `m_` prefix with snake_case (`m_impl`, `m_window_delegate`); headers use `#pragma once`.
- Commit messages must follow [Conventional Commits](https://www.conventionalcommits.org/): `<type>(<optional scope>): <description>`, e.g. `feat(platform): add window resize callback`, `fix(darwin): release window delegate`. Common types: `feat`, `fix`, `refactor`, `build`, `docs`, `chore`, `test`, `perf`. Mark breaking changes with `!` after the type/scope or a `BREAKING CHANGE:` footer.
