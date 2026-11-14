# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & run

CMake (≥ 3.24), C++20, one preset per host OS (Debug, output in `build/<preset>/`):

- `macos`: Apple Clang.
- `windows`: Ninja + `g++` from `PATH` (CLion's bundled MinGW works: put its `bin/cmake/win/x64/bin`, `bin/ninja/win/x64` and `bin/mingw/bin` on `PATH`).

```bash
cmake --preset macos
cmake --build --preset macos
ctest --preset macos
./build/macos/src/main
```

The Vulkan RHI backend (`MANGO_RHI_VULKAN`, default ON) needs **no Vulkan SDK**: `cmake/VulkanDependencies.cmake` fetches Vulkan-Headers, volk and VMA at pinned versions (keep Vulkan-Headers and volk on the same `vulkan-sdk-*` tag). volk loads the GPU driver's Vulkan loader at runtime, so nothing links against it; their implementations are compiled once in `src/rhi/vulkan/*Implementation.cpp`. Do not add `find_package(Vulkan)` or other SDK lookups. Configure with `-DMANGO_RHI_VULKAN=OFF` to build without the backend.

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

## Conventions

- Member fields use the `m_` prefix with snake_case (`m_impl`, `m_window_delegate`); headers use `#pragma once`.
- Commit messages must follow [Conventional Commits](https://www.conventionalcommits.org/): `<type>(<optional scope>): <description>`, e.g. `feat(platform): add window resize callback`, `fix(darwin): release window delegate`. Common types: `feat`, `fix`, `refactor`, `build`, `docs`, `chore`, `test`, `perf`. Mark breaking changes with `!` after the type/scope or a `BREAKING CHANGE:` footer.
