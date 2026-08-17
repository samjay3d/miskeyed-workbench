# Workbench

**Workbench** (distributed on PyPI as `miskeyed-workbench`, imported as `miskeyed.workbench`)
is a **native Qt 6.8 / QRhi Slang shader workbench**. Shaders are compiled in-process
through Slang's compilation API, rendered with QRhi, and driven by a live,
reflection-based parameter UI.

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
```

Or from Python:

```python
from miskeyed.workbench import WorkbenchWindow
```

> Binary wheels are Windows / Direct3D 11. Building from source needs the Qt 6.8
> and Slang SDKs — see [Building from source](#building-from-source).

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
                          Direct3D 11 (QRhi)
```

QRhi is the rendering abstraction, so the Vulkan / Metal / D3D12 backends remain
reachable; the shipped build targets Direct3D 11.

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
#include <slang_qrhi/ShaderDocument.h>
#include <slang_qrhi/SlangRhiWidget.h>

using namespace slang_qrhi;   // internal C++ namespace

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
- Python 3.11+
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
  -DSLANG_QRHI_BUILD_APP=ON
cmake --build build --config Release
```

## Files that matter

```text
cpp/include/slang_qrhi/
    DependencyGraph.h       live dependency DAG + dirty propagation
    ShaderParameterModel.h  reflected GPU parameter model
    ParameterInspector.h    automatic Qt parameter controls
    ShaderDocument.h        source/compile/state coordinator
    SlangRhiWidget.h        embeddable QRhiWidget
    WorkbenchWindow.h       standalone workbench composition

cpp/src/
    SlangCompiler.cpp       in-process Slang API
    Qt68ShaderBridge.cpp    Slang output/reflection -> QShader
    DependencyGraph.cpp     incremental invalidation
    SlangRhiWidget.cpp      QRhi rendering + cheap buffer updates

bindings/
    typesystem_slang_qrhi.xml

app/
    main.cpp                native executable entry point

python/miskeyed/workbench/
    __init__.py             Shiboken module exposure
    __main__.py             `workbench` console entry point
```

## Roadmap

1. reflect Slang user attributes into ranges/groups/widgets;
2. resource reflection model (`Texture2D`, samplers, buffers, HDRI file widgets);
3. graphics/compute pass graph;
4. mesh/camera/environment scene helpers;
5. compile work on a dedicated worker with a long-lived compiler service;
6. persistent disk cache keyed by the dependency DAG + Slang `getEntryPointHash()`
   + render state.

## License

MIT — see [LICENSE](LICENSE).
