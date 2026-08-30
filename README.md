# Workbench

**Edit a shader on the left, see it render on the right — instantly.**

![Workbench: live Slang editing with a rendered viewport and the compiled HLSL side-by-side](https://raw.githubusercontent.com/samjay3d/miskeyed-workbench/main/docs/images/workbench.png)

Workbench is a small desktop tool for **writing shaders and watching them update in
real time**. You type shader code, and the picture on screen recompiles as you go —
no "export, run the compiler, relaunch, look again" loop. It also shows you the
*compiled* output (e.g. HLSL) next to your source, and builds sliders and colour
pickers for your shader's parameters automatically.

> **Heads up — this is a personal side project / experiment.** I built it to see how
> easy a real-time shader-compile loop could feel for artists and TDs. The portable
> shell targets D3D11 or Vulkan on Windows, Vulkan on Linux, and Metal on macOS. Treat it as a
> playground and a proof of concept, not a supported product.

### New to Slang? Read this first

The shaders here are written in **[Slang](https://shader-slang.com/)**, a modern shader
language from NVIDIA, now governed by Khronos. Its big idea is **write once, run
everywhere**: the *same* shader source can compile to Direct3D (HLSL), Vulkan (SPIR-V),
Metal, and WebGPU — so you don't rewrite shaders for every platform.

If you've never touched Slang, this hands-on walkthrough is the friendliest starting
point (no prior Slang needed):

> 📖 **[Hands-On with Slang: A Practical Tutorial for Graphics Teams](https://blog.4dpipeline.com/hands-on-with-slang-a-practical-tutorial-for-graphics-teams)** — write a
> simple Slang shader, compile it to Metal / HLSL / SPIR-V from one source, and run it
> from Python. About 20 minutes end-to-end.

**Why that tutorial matters for this project:** it shows Slang compiling shaders on
demand from a single source. Workbench takes that idea and makes it *interactive* —
instead of running the `slangc` compiler on the command line each time, it compiles
your shader **in-process, on every keystroke**, and shows the result live. That's the
whole experiment: *how easy and immediate can shader iteration feel* when the compiler
is always on. You can also try Slang with zero install in the
[Slang Playground](https://shader-slang.com/playground) in your browser.

You don't need to understand the internals below to *use* Workbench — open it, type in
the left panel, and watch the right panel. The rest of this README is for people who
want to embed it or build it from source.

---

For the technically curious: **Workbench** (distributed on PyPI as `miskeyed-workbench`,
imported as `miskeyed.workbench`) is a **native Qt 6.8 / QRhi Slang shader workbench**.
Shaders are compiled in-process through Slang's compilation API, rendered with QRhi,
and driven by a live, reflection-based parameter UI.

The same C++ Qt objects power three surfaces:

- the standalone `workbench` desktop application;
- a PySide6 tool through Shiboken6 bindings;
- a DCC or host app that embeds the exposed `SlangRhiWidget`, `ShaderDocument`,
  `ShaderParameterModel`, or `ParameterInspector`.

There is no `slangc` subprocess, no `qsb` subprocess, and no Python-owned renderer:
Qt ownership, signals/slots, parameter buffers, dependency tracking, Slang sessions,
and QRhi resources all remain native.

## Install

```powershell
pip install miskeyed-workbench
workbench                 # launch the standalone app
workbench eye.slang       # open a shader on start
uvx miskeyed-workbench    # one-shot launch
```

Then just start typing in the shader panel — edits recompile and re-render live.
Not sure what to type? Try the [Slang tutorial](https://blog.4dpipeline.com/hands-on-with-slang-a-practical-tutorial-for-graphics-teams)
or the [Slang Playground](https://shader-slang.com/playground) for shader snippets you can paste in.

Or from Python:

```python
from miskeyed.workbench import WorkbenchWindow
```

Binary wheels are built for CPython 3.11–3.13 on Windows x86_64, Linux x86_64,
and macOS arm64/x86_64. They bundle the matching Slang runtime; Qt and Shiboken come from
the declared PySide6 dependencies. Source builds need the Qt 6.8 and Slang SDKs —
see [Building from source](#building-from-source).

## Architecture

```text
                         ShaderDocument (QObject)
                                  |
                  +---------------+----------------+
                  |                                |
            DependencyGraph                  SlangCompiler
            live dependency DAG               C++ API only
                  |                                |
        semantic dirty propagation       IGlobalSession / ISession
                  |                                |
       +----------+----------+              Slang reflection
       |          |          |                     |
       v          v          v                     v
    Qt UI      uniform     pipeline       ShaderParameterModel
    only       update      rebuild          dynamic controls
       |          |          |                     |
       +----------+----------+---------------------+
                                  |
                            SlangRhiWidget
                              QRhiWidget
                                  |
                          D3D11 / Vulkan / Metal (QRhi)
```

QRhi is the rendering abstraction. `Auto` selects D3D11 on Windows, Vulkan on Linux,
and Metal on macOS; the native executable also accepts `--rhi vulkan` on Windows.
This live backend is independent of the HLSL / SPIR-V / Metal generated-code tab.

## Runtime: zero compiler subprocesses

Shader compilation runs through Slang's in-process compilation API:

```text
source buffer
   -> IGlobalSession / ISession
   -> loadModuleFromSourceString()
   -> entry points
   -> link()
   -> getEntryPointCode()
   -> SPIR-V + HLSL + MSL blobs in memory
   -> QShader
   -> QRhiGraphicsPipeline
```

`slangc` and `qsb` are not invoked at runtime.

The Qt 6.8 bridge constructs `QShader` directly from Slang output. Because
`QShaderDescription` does not expose public mutation APIs for reflection metadata,
the bridge is deliberately isolated in `Qt68ShaderBridge.cpp` and uses Qt's private
`QShaderDescriptionPrivate`. That is acceptable here because QRhi itself already
carries Qt-minor-version compatibility constraints. When moving to Qt 6.9/6.10,
this file is the compatibility seam.

## Dynamic parameter UX

The parameter model is driven by **Slang reflection**.

Add a numeric global shader parameter:

```slang
float pupilDilation;
float corneaIOR;
float3 irisPigment;
bool debugCornea;
```

After the shader recompiles, `ShaderParameterModel` reflects the parameter layout
and `ParameterInspector` rebuilds automatically. Existing values are preserved
across hot reload when name and type remain compatible.

User-defined Slang attributes (`UIRange`, `UIGroup`, `UIColor`, `UIFile`, etc.)
are exposed through reflection, so ranges and widgets stay shader-owned without
comment parsing.

## Dependency graph / invalidation model

Invalidation is tracked by a live dependency graph. Each node contains:

- stable key;
- node kind;
- local payload digest;
- dependency list;
- Merkle digest;
- dirty / work flags.

Dependencies are canonicalized by stable key before hashing. The graph is a DAG,
so shared shader modules/resources are represented once rather than copied into a
tree.

Hash identity and required work are intentionally separate concepts:

```text
ParameterValues changed -> UniformDirty
UiSchema changed        -> UiDirty
Resource changed        -> ResourceDirty
BindingLayout changed   -> BindingDirty + PipelineDirty
Source/Module changed   -> ShaderDirty + PipelineDirty
```

This keeps common interactions cheap:

```text
slider drag     -> dynamic uniform-buffer update only
texture content -> resource upload only
UI metadata     -> rebuild inspector only
shader body     -> compile affected program/pipeline
binding change  -> rebuild bindings + pipeline
```

Digests use a 32-byte BLAKE2b implementation matching
`hashlib.blake2b(..., digest_size=32)`.

## Native C++ API

```cpp
#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <miskeyed/workbench/rendering/SlangRhiWidget.h>

using namespace miskeyed::workbench::slang_rhi;   // internal C++ namespace

auto* doc = new ShaderDocument(parent);
doc->setFileUrl(QUrl::fromLocalFile("eye.slang"));
doc->load();
doc->compile();

auto* viewport = new SlangRhiWidget(parent);
viewport->setDocument(doc);
layout->addWidget(viewport);
```

## PySide6 / Shiboken6 API

The Python module exposes the same QObject/QWidget classes:

```python
from miskeyed.workbench import ShaderDocument, SlangRhiWidget, ParameterInspector

self.doc = ShaderDocument(self)
self.doc.fileUrl = QUrl.fromLocalFile("eye.slang")
self.doc.load()

self.viewport = SlangRhiWidget(self)
self.viewport.document = self.doc

self.inspector = ParameterInspector(self)
self.inspector.model = self.doc.parameters
```

There is no Python mirror of the render core.

## Building from source

Requirements:

- C++20
- Qt **6.8.x** SDK, including private QtGui headers (`Qt6::GuiPrivate`)
- PySide6 **6.8.x**
- Shiboken6 **6.8.x** generator
- Slang SDK (set `SLANG_ROOT` if CMake cannot find it)
- Python 3.11–3.13
- CMake 3.24+

For VFX Platform 2026 deployments, build against the exact Qt/PySide toolchain
used by the host DCC.

### Shiboken generator

Qt's PyPI `shiboken6` package is the runtime module; the generator is distributed
by Qt separately. Install the matching generator from Qt's official wheel index
before building the Python extension:

```powershell
python -m pip install `
  --index-url https://download.qt.io/official_releases/QtForPython/ `
  --trusted-host download.qt.io `
  PySide6==6.8.* shiboken6==6.8.* shiboken6_generator==6.8.*
```

Then point CMake at your Qt 6.8 development SDK and Slang SDK, and build the wheel:

```powershell
$env:SLANG_ROOT = "C:\sdk\slang"
$env:CMAKE_PREFIX_PATH = "C:\Qt\6.8.3\msvc2022_64"

pip install --no-build-isolation .
```

Or build the native app directly with CMake:

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64 `
  -DSLANG_ROOT=C:\sdk\slang `
  -DMISKEYED_WORKBENCH_BUILD_APP=ON
cmake --build build --config Release
```

## Files that matter

```text
cpp/include/miskeyed/workbench/
    core/                   identity and deterministic evaluation contracts
    editor/                 code editor and language-service API
    slang/                  compiler, reflection, documents, workspace, modules
    rendering/              QRhi pass and viewport API
    ui/                     reusable parameter inspectors
    modes/render_toy/       Render Toy window/composition API
    anari/                  optional ANARI host API

cpp/src/workbench/          implementations in the same responsibility layout
shaders/workbench/          packaged Workbench Slang module sources
docs/SOURCE_LAYOUT.md       ownership and dependency direction

bindings/
    typesystem_workbench.xml

app/
    main.cpp                native executable entry point

python/miskeyed/workbench/
    __init__.py             Shiboken module exposure
    __main__.py             `workbench` console entry point
```

Workbench-owned Slang contracts live in `shaders/workbench/` and are packaged with the
native target. The editor treats generated text as a document view, while the inspector
separates reflected parameters and compilation from a live dependency view. That view
walks the focused document's `DependencyGraph`; selecting a node shows the current module
or source payload, and edits to imported project files update it after recompilation.

CI also captures the native window after compilation and its first rendered frames. The
`workbench-documentation` artifact provides the current screenshot for review and
promotion into [`docs/images`](docs/images/README.md) as the interface evolves.

## Roadmap

### Experimental ANARI backend probe

ANARI work is isolated from the shipped application and defaults to off. Backend
developers can build the headless probe without Qt, Slang, or the Workbench target:

```powershell
cmake -S spikes/anari_probe -B build/anari-probe `
  -DCMAKE_PREFIX_PATH=C:\sdk\anari
cmake --build build/anari-probe --config Release
```

The probe tries an explicit semicolon-separated candidate list passed as its first
argument, then `MISKEYED_ANARI_LIBRARIES`, then the built-in development candidates.
Each candidate is an ANARI loader name or the SDK's `name,path` form. See
[`docs/anari/COMPATIBILITY.md`](docs/anari/COMPATIBILITY.md) and the
[`ANARI implementation breakdown`](docs/ANARI_HOST_IMPLEMENTATION_PLAN.md).

The root build exposes the same backend behind
`MISKEYED_WORKBENCH_WITH_ANARI=ON`. Leaving it off preserves the existing dependency
graph and runtime behavior.

1. reflect Slang user attributes into ranges/groups/widgets;
2. resource reflection model (`Texture2D`, samplers, buffers, HDRI file widgets);
3. graphics/compute pass graph;
4. mesh/camera/environment scene helpers;
5. compile work on a dedicated worker with a long-lived compiler service;
6. persistent disk cache keyed by the dependency DAG + Slang `getEntryPointHash()`
   + render state.

## License

MIT — see [LICENSE](LICENSE).
