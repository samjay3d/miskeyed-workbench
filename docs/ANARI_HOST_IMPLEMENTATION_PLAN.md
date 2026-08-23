# ANARI Device mode research and design plan

## Status and evidence boundary

This document plans Miskeyed's third work mode, **ANARI Device**. It does not redefine
the whole application as an ANARI host and it does not make Island the product.

Miskeyed has three modes with increasing scope:

1. **Shader Toy** — one live Slang shader and reflected parameters;
2. **Render Toy** — Shader Toy with a scene pre-pass and post-process pass; and
3. **ANARI Device** — real USD/Hydra scene work through selectable ANARI devices.

Modes 1 and 2 are the shipped Slang/QRhi workbench. Mode 3 is a design and research
track whose UX must be derived from actual function, capability, and delta data. It
must not distort the simple modes while that design is unsettled.

The upstream review for this revision used these source snapshots:

- ANARI-SDK `7897cbe425e5000ce890631bc97d14e00c750175` (including `hdAnari`,
  Helide, Helium, debug, sink, remote, examples, and CTS);
- VisRTX `2eec808bb6676767c4cf393ba580142026ec384e`;
- TSD in VisRTX `next_release` at
  `8f31d491131544090257f05cac11f988ca428f77`; and
- Island `ad5ccd073652704aa0dd0461bdf38346e1354a7e`.

These moving snapshots inform the research plan; the first build spike still must pin
a mutually compatible release set for Miskeyed's supported environment.

Primary upstream references:

- [TSD overview](https://github.com/NVIDIA/VisRTX/tree/next_release/tsd)
- [TSD `ANARIDeviceManager`](https://github.com/NVIDIA/VisRTX/blob/next_release/tsd/src/tsd/app/ANARIDeviceManager.cpp)
- [TSD device-switching viewport](https://github.com/NVIDIA/VisRTX/blob/next_release/tsd/src/tsd/ui/imgui/windows/Viewport.cpp)
- [TSD ANARI capture device](https://github.com/NVIDIA/VisRTX/tree/next_release/tsd/src/anari_tsd)
- [ANARI-SDK frontend and implementation guide](https://github.com/KhronosGroup/ANARI-SDK/tree/next_release/src/anari)
- [`hdAnari` render delegate](https://github.com/KhronosGroup/ANARI-SDK/tree/next_release/src/hdanari)

## Product statement

ANARI Device mode should give technical artists a neutral environment in which they
can:

- open a USD scene and preserve its authored semantics;
- author non-destructive USD look overrides;
- observe Hydra dirty propagation and the resulting ANARI operations;
- switch between multiple ANARI implementations;
- compare renderer behavior without changing scene ownership; and
- experiment with Slang independently of scene ingestion.

```text
USD / Hydra scene
        +
Slang shader experimentation
        +
switchable ANARI devices
        =
technical-art look development and renderer debugging
```

The first architectural proof is not merely rendering USD in Qt. It is one USD stage
driving multiple interchangeable ANARI devices while incremental scene changes remain
observable and shader-authoring state remains independent. Which panes, workflows, and
editing affordances make that useful is deliberately still a design question.

## Relationship to the other two modes

| Mode | Input | Native execution | Intended use | Scene authority |
| --- | --- | --- | --- | --- |
| Shader Toy | One Slang program | In-process Slang + one QRhi pass | Fast shader experiments | None |
| Render Toy | Scene-pass Slang + post-pass Slang | In-process Slang + QRhi offscreen/final passes | Small rendering experiments | Shader-authored procedural scene |
| ANARI Device | USD/Hydra plus selected ANARI implementation | Hydra/hdAnari + device; separate Slang authoring surface | Look development, device comparison, delta inspection | USD stage/session layer |

ANARI Device mode sits beside the existing modes. It does not turn `SlangRhiWidget`
into an external-renderer container and does not turn the Render Toy into a USD
renderer. Shared UI concepts should be extracted only after a working vertical slice
proves they are truly shared.

## Target architecture

```text
                         TECH ARTIST
                              |
              +---------------+----------------+
              |                                |
        USD scene/look edits             Slang authoring
              |                                |
              v                                v
         SceneDocument                   ShaderDocument
              |                                |
              v                                |
            Hydra                              |
              |                                |
              v                                |
           hdAnari                             |
              |                                |
              +---------------+----------------+
                              v
                         ANARI HOST
                              |
          +-------------------+--------------------+
          |                   |                    |
          v                   v                    v
       Helide              VisRTX           SDK debug / TSD
      baseline           GPU renderer      trace and capture

                              future
                                |
                                v
                      Island ANARI device
                                |
                                v
                         Island + Slang
```

The line from `ShaderDocument` to the host is a research seam, not an MVP feature. The
portable look path should first use USD/MaterialX semantics that devices can advertise
and implement independently. A future Island or other Slang-capable device may expose
a device-specific extension for lower-level programmable materials, hair, eyes, cloth,
or geometry. Until that contract exists, shader authoring and ANARI rendering remain
two visible but independent paths, and the UI must label device-specific Slang behavior
as non-portable rather than implying every ANARI device can consume it.

## Non-negotiable ownership boundaries

### USD and Hydra path

```text
USD delta
  -> Hydra change/dirty propagation
  -> hdAnari
  -> ANARI object and parameter operations
```

- USD remains the authored source of truth.
- Hydra owns scene change propagation.
- `hdAnari` owns Hydra-to-ANARI translation.
- Miskeyed does not traverse USD into a private render scene.
- Miskeyed does not mirror Hydra dependency state.

### Slang path

```text
Slang source delta
  -> ShaderDocument
  -> in-process compile and reflection
  -> shader product and pipeline invalidation
```

- `ShaderDocument` remains shader-specific.
- ANARI library/device selection is not shader state.
- USD ownership is not shader state.
- A future device may consume a shader product through an explicit edge API, but it
  must not take ownership of the compiler session.

### Portable look versus programmable renderer features

Do not force one material mechanism to answer two different needs:

1. **Portable look path:** USD/MaterialX-authored materials translated through Hydra and
   `hdAnari` into standard or advertised ANARI material capabilities. This is the path
   used for meaningful device comparison.
2. **Programmable device path:** explicit device extensions for experiments that need
   lower-level Slang control over a renderer-specific hair, eye, cloth, geometry, or
   material model. This path is valuable but not portable by default.

Research whether MaterialX generation can produce the portable representation while a
device extension exposes an advanced Slang implementation behind a clear capability
check. Never silently compare a device-specific programmable model with a portable
fallback as though they were equivalent. The UI must show the active material path,
unsupported features, and degradation.

ANARI scene objects describe data and renderer-facing intent; they do not by themselves
make arbitrary GPU-generated geometry exportable. If a Slang experiment generates or
deforms geometry solely on a device, exporting it requires an explicit readback/bake
contract supplied by that device. Do not promise that opaque device-internal GPU work
can become USD automatically.

### UI and presentation path

- Qt owns application UI and presentation.
- An ANARI device owns its renderer resources.
- The generic scene viewport initially maps a frame color channel and uploads it for
  display.
- Device-specific native texture interop is deferred until profiling justifies it.
- Python/PySide may expose native objects but must not own USD, Hydra, ANARI, or render
  lifecycles.

## Repository assessment

### Reusable components

| Existing component | Reuse | Boundary |
| --- | --- | --- |
| `ShaderDocument` | Keep unchanged in purpose | Slang source, compile, reflection, generated shaders, shader dependency state |
| `DependencyGraph` | Reuse for shader products; possibly use its principles for host records | Do not duplicate Hydra's dirty graph in it |
| `ShaderParameterModel` and `ParameterInspector` | Keep for reflected Slang parameters | Do not use as an implicit USD material model |
| `WorkbenchWindow` | Composition root for an initial UI spike | Must stop calling a second shader document the scene document |
| `SlangRhiWidget` | Keep as the shader viewport initially | Do not force ANARI devices or Island through its QRhi renderer |
| Shiboken build | Later expose stable `SceneDocument`/host metadata | Native lifecycle stays in C++ |

### Naming conflict

`WorkbenchWindow::sceneDocument()` currently returns `ShaderDocument*`, and
`m_sceneDocument` is another shader pass. Before adding the real scene object, rename
these to names reflecting their actual role, such as `sceneShaderDocument()` and
`m_sceneShaderDocument`. No compatibility shim is required.

### Build-system effect

The current core target directly requires Qt and Slang. OpenUSD/Hydra, ANARI, and
`hdAnari` must initially be optional build features so the existing shader workbench
continues to build without those SDKs. Do not hard-code SDK paths. Add CMake discovery
only after the compatibility spike identifies exported package targets and runtime
layout.

### Risky current assumptions

1. `hdAnari` is available and compatible with the selected OpenUSD build.
2. Helide and VisRTX build and load through the selected stable ANARI frontend.
3. Switching the device used by `hdAnari` can repopulate renderer state without
   reopening the USD stage in application code.
4. `hdAnari` exposes enough hooks to associate Hydra dirty events with captured ANARI
   operations.
5. VisRTX supports the Windows/toolchain target used by the Workbench.
6. A mapped color channel has an image format and row layout that the generic Qt
   presenter can handle across all selected devices.
7. A future Island device can coexist with the Workbench Slang runtime.

These are spike questions, not facts to build around.

## Upstream compatibility matrix to pin

The first research deliverable is a checked-in lock table populated from actual
upstream commits and build output:

| Component | Current requirement | Evidence required | Stop condition |
| --- | --- | --- | --- |
| Qt/PySide/Shiboken | 6.8.x in this package | Current CMake and wheel build | Host requires an incompatible event/UI toolchain |
| Slang | Environment-provided SDK; current compiler code uses native API | Exact installed version and Island requirement | Two incompatible Slang runtimes must load in one process |
| OpenUSD | Unpinned | Commit/release exporting required Hydra host APIs | `hdAnari` requires an incompatible Hydra generation |
| ANARI-SDK | Use the latest release or deliberately track `next_release` | Confirm exported targets and selected branch policy | Required host behavior exists only on an unshippable fork |
| `hdAnari` | Unpinned | Repository, target, compatible OpenUSD and ANARI commits | Cannot inject/select the intended ANARI library/device |
| Helide | Expected from ANARI-SDK; verify | Target and runtime library name | Cannot load through the same host API |
| VisRTX | Optional interactive device | Supported OS/GPU/toolchain and load test | Requires an unavailable GPU stack for baseline CI |
| TSD | Reference and optional capture tooling, not a core dependency | Device manager, viewport, and `anari_tsd` behavior at the chosen VisRTX commit | Reuse would introduce another authoritative scene database |
| Island | Future research only | Commit, Slang/Vulkan requirements, extension seams | Requires host layers to depend on Island internals |

Store the resulting matrix in `docs/anari/COMPATIBILITY.md` with exact repository URLs,
commits, CMake targets, loadable library/device names, extension lists, and runtime
DLLs. CI may exercise only Helide initially; VisRTX can be capability-gated.

## Actual ANARI host API surface to verify

The host should use the ANARI API directly rather than hiding it behind a general scene
abstraction. The public API operations expected by the first spike are:

```text
anariLoadLibrary
anariUnloadLibrary
anariGetDeviceSubtypes
anariGetDeviceExtensions
anariNewDevice
anariGetObjectSubtypes
anariGetObjectInfo
anariGetParameterInfo
anariSetParameter
anariCommitParameters
anariRelease
```

The build spike must verify every signature and availability against the selected
pinned `<anari/anari.h>` and extension headers.

### Source-confirmed loader behavior

The reviewed ANARI-SDK provides the following concrete deployment seam:

- `anariLoadLibrary(name, callback, userData)` dynamically loads a library named
  `anari_library_[name]` (with the platform prefix/suffix) and resolves the matching
  `anari_library_[name]_new_library` entry point.
- A library can be selected indirectly with `anariLoadLibrary("environment", ...)`,
  which reads `ANARI_LIBRARY`.
- The loader first performs an ordinary platform library lookup, then tries beside the
  ANARI frontend library. It also accepts the SDK's `name,path` form for an explicit
  library location.
- `anariGetDeviceSubtypes()` and `anariGetDeviceExtensions()` provide the first level
  of capability metadata; object and parameter query APIs provide the deeper surface.
- `anariUnloadLibrary()` deletes the implementation object before unloading its OS
  module, so every object and device from that library must be gone first.

Sources to recheck at the pinned commit:

- ANARI-SDK `src/anari/README.md`, “Implementing ANARILibrary”;
- `src/anari/LibraryImpl.cpp`, `loadANARILibrary()`;
- `src/anari/API.cpp`, `anariLoadLibrary()` and `anariUnloadLibrary()`; and
- `src/anari/include/anari/anari.h`, initialization and introspection declarations.

The SDK already exports `anari::anari`, `anari::anari_static`, and `anari::helium`
CMake targets. Miskeyed should consume the exported frontend target rather than add a
custom `FindANARI.cmake` unless the selected packaged SDK demonstrably lacks its config.

### Source-confirmed `hdAnari` constraint

The reviewed `HdAnariRenderDelegate::Initialize()` currently loads
`anari::loadLibrary("environment", ...)`, queries the `"default"` device subtype, and
creates `anari::newDevice(library, "default")`. Consequently:

- the selected library currently comes from the process-wide `ANARI_LIBRARY` value;
- the selected device subtype is currently fixed to `default`;
- changing library safely means recreating the Hydra render delegate and all of its
  device-side ANARI state; and
- changing `ANARI_LIBRARY` concurrently for multiple delegates is unsafe as a host
  design because environment state is process-global.

This is the most important function-level result discovered so far. The MVP must not
pretend arbitrary in-process selection is already exposed by `hdAnari`.

Preferred resolution: contribute or maintain the smallest upstreamable `hdAnari`
change that accepts library name/path and device subtype through render-delegate create
arguments or initial settings, then stores that selection per delegate. Do not fork its
USD translation and do not create a subprocess merely to work around selection when a
native injection seam can be added.

Relevant upstream symbols:

- `HdAnariRendererPlugin::CreateRenderDelegate()`;
- `HdAnariRenderDelegate(HdRenderSettingsMap const&)`;
- `HdAnariRenderDelegate::Initialize()`; and
- `HdAnariRenderParam`, which retains the shared ANARI device and scene state.

### Discovery versus enumeration

Do not assume ANARI globally discovers every installed library. The plan should treat
library candidates as host configuration obtained from:

1. explicit application settings or command-line values;
2. an environment/configured search path supported by the pinned loader; and
3. an optional packaged manifest generated at install time.

For each candidate library, the host loads it, enumerates device subtypes, queries
extensions/capabilities, then unloads it when no device or probe retains it. The exact
loader filename convention and environment variables must come from the pinned
ANARI-SDK source, not from a Miskeyed convention.

## MVP device discovery and deployment

ANARI deliberately has no central device registry. TSD demonstrates a suitably small
application policy: start with a fixed candidate list (`helide`, `visrtx`, and other
known development devices), allow a comma-separated environment variable to replace
the list, and load candidates on demand through the standard ANARI loader.

Miskeyed should begin with the same scale of mechanism:

```text
built-in candidates: helide, helide_gpu, visrtx, debug
override: MISKEYED_ANARI_LIBRARIES=name[,path];name[,path];...
user settings: the same ordered candidate strings, persisted by Qt
```

The precise separator must avoid colliding with the ANARI SDK's existing `name,path`
syntax. Candidate strings are library identities or explicit loader locations, not a
claim that Miskeyed discovered every installed renderer.

### MVP deployment flows

1. **Use an existing installation.** Add its library name or explicit `name,path`
   candidate. The vendor/Rez environment remains responsible for dependent DLLs.
2. **Ship a development bundle.** Package the ANARI frontend, Helide, and their required
   runtime files together for CI and first-run validation.
3. **Register a studio environment.** Let Rez or another package environment populate
   `MISKEYED_ANARI_LIBRARIES` and native library paths before Workbench starts.

Do not build a marketplace, universal registry, signed archive format, or second plugin
ABI for the MVP. If later deployment needs provenance and license metadata, add a small
optional manifest around the existing ANARI library identity; do not copy ANARI
capability declarations into it.

### Discovery safety and lifecycle

- Treat each candidate as native plugin code and load it only on explicit probe or
  activation, not merely to paint a cached menu.
- Cache the result of `anariGetDeviceSubtypes()`, extension queries, and probe failures
  for the current environment.
- Keep one selected device session per ANARI Device workspace for the MVP.
- Activate a replacement session before retiring the current one when the upstream
  `hdAnari` selection seam permits it.
- Never unload a library until its Hydra delegate, frames, device, and every object from
  that library are destroyed.
- A failed candidate must not prevent Shader Toy or Render Toy from launching.
- Keep VisRTX and future Island runtime dependencies outside the base wheel by default.

### Reference implementation in TSD

Use TSD as source guidance, not as a dependency or a new scene authority:

- `tsd/src/tsd/app/ANARIDeviceManager.{h,cpp}` maintains a candidate list, loads
  libraries/devices lazily, caches extensions, retains devices, and unloads libraries
  last.
- `TSD_ANARI_LIBRARIES` replaces its default candidates; this is the direct precedent
  for Miskeyed's MVP configuration.
- `tsd/src/tsd/ui/imgui/windows/Viewport.cpp` tears down and repopulates device-side
  render state when a library changes and provides failure fallback behavior.
- `MultiDeviceViewport.cpp` is useful research for multiple devices, but its tethered
  multi-device design is not automatically Miskeyed's A/B comparison design.

TSD also contains its own scene and render-index model. Miskeyed must not import that
layer into the USD/Hydra path: USD remains authoritative and `hdAnari` remains the
translator.

### Minimal native abstraction

Use small ownership types rather than a renderer facade:

```text
AnariLibrary
  - loaded library handle
  - configured library name/path identity
  - available device subtype metadata
  - status callback bridge

AnariDeviceSession
  - strong reference to AnariLibrary
  - device handle and subtype
  - extension/capability snapshot
  - renderer/world/frame handles owned for the active Hydra delegate
  - deterministic teardown

AnariDeviceCatalogModel
  - read-only Qt model of probed candidates and failures
  - stable selection key: library identity + device subtype
```

These types manage handles and metadata; they do not wrap geometry, materials, or the
rest of ANARI into custom Miskeyed scene objects.

### Lifecycle

```text
probe library
  -> load library
  -> enumerate subtypes/extensions
  -> expose metadata or diagnostic

activate selection
  -> create new device session
  -> configure status callback
  -> create/recreate Hydra hdAnari delegate state
  -> populate from the existing USD/Hydra scene
  -> publish new frame source
  -> retire old delegate/device/frame safely
  -> unload old library after its final device/object release
```

Device switching is allowed to rebuild all device-side ANARI objects. It must not
reopen or manually reinterpret the USD stage. Whether the Hydra render index/delegate
must be recreated is an upstream fact to document during Stage 3.

## Diagnostics and ANARI tracing are product features

The trace must make these calls observable:

```text
anariNew*
anariSetParameter
anariUnsetParameter
anariCommitParameters
anariRelease
anariRenderFrame
anariMapFrame / anariUnmapFrame
array creation and replacement
object handles and stable trace identities
```

Do not display raw pointer values as durable identity. Assign monotonic, per-session
trace IDs while retaining the raw handle only as diagnostic detail.

### Event model

```text
AnariTraceEvent
  sequence
  timestamp
  thread
  operation
  trace object ID
  ANARI object type/subtype
  parameter name/type
  summarized old/new value
  originating device session
  optional Hydra correlation
```

Large arrays and images must be summarized by type, dimensions, byte size, and a
content digest rather than copied into the UI log.

### Instrumentation decision

Investigate in this order:

1. ANARI-SDK's existing debug device, including its code trace mode;
2. TSD's `anari_tsd` capture/pass-through device for offline object and parameter
   inspection;
3. `hdAnari` debug codes or logging for Hydra-side correlation;
4. upstream improvements to those tools; and
5. only then, a small dispatch interposer owned by Miskeyed.

Do not permanently fork `hdAnari` merely to log calls. The chosen mechanism must see
the calls made by `hdAnari`, not just calls made by the host around it.

The reviewed TSD capture device mirrors ANARI object state, writes `live_capture.tsd`,
and can forward rendering to a real backend selected by `ANARI_TSD_LIBRARY`. It is
optional research tooling: a captured final scene does not by itself prove incremental
call ordering or unchanged identity.

Target output:

```text
USD roughness changed

Hydra
  Material /Looks/body dirty

ANARI
  material #18
    set roughness: 0.4 -> 0.2
    commit

unchanged
  geometry #5
  array #2
  instance #9
```

## UX research before the ANARI workspace design

The ANARI Device mode UX is intentionally unresolved. Do not begin by adding a third
large tab that simply looks like a USD viewer. First collect functional data from the
fixture, devices, and trace layer, then answer these workflow questions with a small
interactive prototype:

1. Is the primary unit a scene document, a comparison workspace, or a captured delta?
2. Does a technical artist normally view one device, A/B two devices, or inspect one
   device plus its ANARI trace?
3. Which device differences need synchronized camera/time and which settings must stay
   device-local?
4. How are unsupported objects, degraded materials, and extension differences surfaced
   without overwhelming the viewport?
5. Does selection originate in the USD hierarchy, an image ID channel, or either?
6. How does a user move from a USD material delta to the exact ANARI parameter operation
   and then to device diagnostics?
7. Where can a separate Slang experiment be attached without implying that every ANARI
   implementation can consume arbitrary Slang?

### Candidate workspace, not a committed UI

```text
+----------------------+------------------------+----------------------+
| USD hierarchy/look   | synchronized viewport | device + render      |
| session layer edits  | one device or A/B      | settings/capability  |
+----------------------+------------------------+----------------------+
| Hydra delta          | ANARI operation trace  | diagnostics          |
+----------------------+------------------------+----------------------+
| optional separate ShaderDocument experiment                         |
+---------------------------------------------------------------------+
```

The first prototype needs only device selection, one viewport, stage identity, and an
operation trace. Add comparison layout and look editing only after observing the
initial population and delta data. Preserve the ability to launch Shader Toy and Render
Toy without loading OpenUSD or any ANARI device.

### UX evidence deliverable

Before finalizing `SceneDocument` or `WorkbenchWindow` composition, check in
`docs/anari/ANARI_DEVICE_UX.md` containing:

- the jobs-to-be-done for a technical artist and a renderer developer;
- screenshots/wireframes for single-device, compare, and trace-focused workflows;
- device-switch and failure-state behavior;
- which state is shared across devices and which is local;
- observed trace examples for every fixture delta; and
- the chosen smallest useful workspace with rejected alternatives.

That document is a gate for production UI work, not post-hoc documentation.

## First fixture and delta matrix

Create a deterministic textual USD fixture containing:

- one indexed mesh;
- one transform;
- one perspective camera;
- one directional or environment light;
- one Preview Surface material;
- one base-color texture; and
- one separate look override layer.

Run these changes independently and capture the ANARI trace:

| Change | Expected minimum effect | Must remain unchanged |
| --- | --- | --- |
| Initial population | Complete object population and first frame | N/A |
| Roughness | Material parameter set and material recommit | Geometry, arrays, stage, `ShaderDocument` |
| Base color | Material parameter set and material recommit | Geometry, arrays, stage, `ShaderDocument` |
| Texture content/path | Sampler/image update or replacement as required | Topology and unrelated materials |
| Transform | Instance/transform parameter update and recommit | Geometry arrays and materials |
| Camera | Camera parameter update and recommit | Scene geometry and materials |
| Topology | Affected geometry/arrays rebuilt | Unrelated objects and shader source |

The exact ANARI operations are outputs of the experiment. If `hdAnari` legitimately
uses a different minimal sequence, update the expected trace while preserving the
semantic invariant: a roughness edit must not recreate unchanged geometry or reopen
the stage.

## Existing-device architecture review

Before Island design, inspect exact source files and symbols and complete this table:

| Responsibility | ANARI-SDK helpers | Helide | VisRTX | TSD `anari_tsd` | Renderer-specific? |
| --- | --- | --- | --- | --- | --- |
| ABI dispatch/export | TBD | TBD | TBD | TBD | Usually generic; verify |
| Device/object handles | TBD | TBD | TBD | TBD | Usually generic; verify |
| Reference counting | TBD | TBD | TBD | TBD | Generic candidate |
| Parameter storage/typing | TBD | TBD | TBD | TBD | Generic candidate |
| Commit semantics | TBD | TBD | TBD | TBD | Split; verify |
| Array ownership/deleters | TBD | TBD | TBD | TBD | Generic candidate |
| Status callbacks | TBD | TBD | TBD | TBD | Generic candidate |
| Extension reporting | TBD | TBD | TBD | TBD | Split; verify |
| Geometry/material creation | N/A or helper hooks | TBD | TBD | capture representation | Renderer-specific |
| Render/frame channels | N/A or helper hooks | TBD | TBD | pass-through/capture | Renderer-specific |

Each `TBD` must be replaced with repository, commit, path, symbol, responsibility, and
license. This evidence determines whether a device kit is justified.

## File-level implementation plan

Names below are proposed and should be adjusted only when pinned upstream APIs provide
a concrete reason.

### Stage 0: compatibility and API spike

Create:

- `docs/anari/COMPATIBILITY.md` — promote the reviewed source snapshots into a tested
  release lock with targets, runtime artifacts, extension lists, and supported
  configurations.
- `spikes/anari_probe/CMakeLists.txt`
- `spikes/anari_probe/main.cpp` — load a configured library, list device subtypes and
  extensions, create/release a device, and route status messages.

Modify:

- none of the production runtime.

Test:

- CTest cases for Helide success, unknown-library failure, unknown-subtype failure, and
  clean repeated load/unload where the loader supports it.

Milestone: one command produces machine-readable catalog output for Helide and records
the exact ABI/version evidence.

### Stage 1: optional host foundation

Create:

- `cpp/include/slang_qrhi/AnariLibrary.h`;
- `cpp/src/AnariLibrary.cpp`;
- `cpp/include/slang_qrhi/AnariDeviceCatalogModel.h`;
- `cpp/src/AnariDeviceCatalogModel.cpp`;
- `tests/test_anari_library.cpp`; and
- `tests/test_anari_device_candidates.cpp`.

Modify:

- `CMakeLists.txt` — use upstream `find_package(anari CONFIG ...)`, add
  `SLANG_QRHI_WITH_ANARI`, an optional native target, and tests;
- binding files only after the C++ ownership API stabilizes.

Ownership: `AnariLibrary` owns the library handle; a device session retains its
library. The catalog owns probe results, not active render devices.

Milestone: the native app can probe configured Helide and VisRTX candidates, enumerate
subtypes/capabilities, and show failures without loading USD or creating a viewport.

### Stage 2: fixture and Hydra/hdAnari proof

Create:

- `tests/fixtures/anari/lookdev.usda`;
- `tests/fixtures/anari/lookdev_override.usda`;
- `spikes/hdanari_fixture/CMakeLists.txt`;
- `spikes/hdanari_fixture/main.cpp`;
- `docs/anari/HDANARI_CALL_SURFACE.md`.

Modify:

- `CMakeLists.txt` only through an optional `SLANG_QRHI_WITH_USD_ANARI` feature.

Test:

- render the fixture with Helide;
- render it with VisRTX when capability-gated hardware/runtime is available;
- record device-specific image/channel formats and runtime requirements.

Milestone: the same fixture and Hydra path produce frames from Helide and VisRTX.

### Stage 3: device switching

Create:

- `cpp/include/slang_qrhi/AnariDeviceSession.h`;
- `cpp/src/AnariDeviceSession.cpp`;
- `tests/test_anari_device_switch.cpp`.

Modify:

- the `hdAnari` host spike to retain the USD stage while recreating only the required
  Hydra/ANARI renderer state.

Test:

- Helide -> VisRTX -> Helide selection;
- failed activation leaves the current session usable;
- old library unload occurs only after all its objects are released;
- the USD stage identity remains unchanged across switches.

Milestone: one open stage switches between two device sessions without application
code reopening or traversing the stage.

### Stage 4: tracing and delta proof

Create after selecting the upstream-supported instrumentation seam:

- `cpp/include/slang_qrhi/AnariTraceModel.h`;
- `cpp/src/AnariTraceModel.cpp`;
- `cpp/include/slang_qrhi/AnariTraceEvent.h`;
- `tests/test_anari_trace_model.cpp`;
- `tests/test_hdanari_deltas.cpp`;
- `docs/anari/traces/*.json` golden structural traces where stable.

Do not create a custom dispatch interposer until the source audit rejects existing
layers.

Test each delta in the fixture matrix. Prefer semantic assertions over brittle total
call counts.

Milestone: the roughness trace proves material update/recommit and absence of geometry,
array, stage, or `ShaderDocument` reconstruction.

### Stage 5: SDK debug and optional TSD capture

Modify:

- `docs/anari/COMPATIBILITY.md` with debug/TSD output and limitations; and
- device candidates to make `debug` and optional `tsd` loadable when installed.

Use ANARI-SDK debug tracing first. Evaluate `anari_tsd` capture/pass-through for
offline object inspection. Keep TSD optional and do not make ordinary host startup or
USD ownership depend on it.

Milestone: the same `hdAnari` scene produces an inspectable SDK trace; optionally, a
TSD capture can be opened independently and forwarded through Helide or VisRTX.

### Stage 6: real `SceneDocument`

Create:

- `cpp/include/slang_qrhi/SceneDocument.h`;
- `cpp/src/SceneDocument.cpp`;
- `tests/test_scene_document.cpp`.

Modify:

- `cpp/include/slang_qrhi/WorkbenchWindow.h`;
- `cpp/src/WorkbenchWindow.cpp`;
- `bindings/slang_qrhi_bindings.h` and `bindings/typesystem_slang_qrhi.xml` only after
  native tests pass;
- `CMakeLists.txt`.

Minimum coordination surface:

```text
USD stage URL
working session/override layer
Hydra and hdAnari lifecycle
selected library + device subtype stable key
ANARI renderer/world/frame coordination required by hdAnari
time code
selection
render settings
status and diagnostics
```

It must not own another scene graph, custom USD translation, device GPU internals,
viewport resources, Slang compiler state, or Python renderer state.

Rename the current shader-pass `sceneDocument()` before introducing the new API.

Milestone: a headless event-loop test opens the fixture, selects Helide, receives a
frame-ready signal, switches device where available, and closes safely.

### Stage 7: generic scene viewport

Create:

- `cpp/include/slang_qrhi/AnariFrameWidget.h`;
- `cpp/src/AnariFrameWidget.cpp`;
- `tests/test_anari_frame_conversion.cpp`.

Modify:

- `WorkbenchWindow` to compose the new scene viewport beside the existing shader
  viewport rather than turning `SlangRhiWidget` into a multi-renderer abstraction.

The widget consumes a host-owned immutable frame snapshot containing size, format,
channel metadata, pixels, and sequence number. It owns only presentation resources.
Frame mapping/copying belongs to the host/session side and must not block the UI thread.

Milestone: one widget displays mapped color output from Helide and VisRTX with no
device-specific branch.

### Stage 8: USD look editing

Create:

- `cpp/include/slang_qrhi/UsdLookModel.h`;
- `cpp/src/UsdLookModel.cpp`;
- `cpp/include/slang_qrhi/UsdLookInspector.h`;
- `cpp/src/UsdLookInspector.cpp`;
- `tests/test_usd_look_override.cpp`.

Initial properties: base color and roughness only. `UsdLookModel` authors opinions into
the working USD layer; it does not edit ANARI objects. Hydra and `hdAnari` propagate
the result.

Milestone: saving/reopening the override preserves edits, and traces prove each edit
touches material state without recreating geometry.

### Stage 9: existing-device source comparison

Complete the table in this document and move the cited result to
`docs/anari/DEVICE_IMPLEMENTATION_COMPARISON.md`. Include actual code paths for Helide,
VisRTX, TSD's capture device, and ANARI-SDK helpers, separating ABI/object boilerplate from
renderer-specific work and optional features.

Milestone: the evidence is sufficient to choose helper strategy A, B, or C without
writing Island code.

### Stage 10: Island device design only

Create:

- `docs/anari/ISLAND_DEVICE_DESIGN.md`.

Do not add production Island code yet. The design must use the captured `hdAnari` call
surface and identify:

- fixture-required ANARI types and parameters;
- exact mapping to Island resources/render-graph APIs;
- commit and incremental update behavior;
- explicit Slang material seam;
- frame production and channels;
- graphics-device and resource ownership;
- threading;
- teardown and frames-in-flight lifetime;
- renderer-specific glue size by responsibility, not an arbitrary line target.

Milestone: a source-cited triangle and PBR-fixture design exposes any missing generic
helper before implementation.

### Stage 11: device-adapter decision

Choose in this order:

1. use ANARI-SDK helpers directly;
2. add a small renderer-agnostic convenience layer over them;
3. create a reusable device-kit library only after measured duplication proves it.

If option 2 or 3 is proposed, document at least two concrete adapters that need the
same missing behavior. Do not make the host depend on this device-authoring layer.

Milestone: an architecture decision record names reused upstream helpers, missing
facilities, measured duplication, and the smallest justified addition.

## `SceneDocument` design constraints

The final API must be derived from the Hydra host prototype, not guessed in advance.
Its conceptual responsibility remains:

```text
SceneDocument
  USD stage/session identity
  Hydra + hdAnari lifecycle
  active ANARI selection/session coordination
  time and selection
  render settings
  status and diagnostics
```

Device selection changes renderer implementation only. It must not change stage
ownership, look-authoring behavior, `SceneDocument` meaning, or `ShaderDocument`
ownership.

The active session may recreate device-side world/frame/renderer objects and the Hydra
delegate/render index if required. Those are consequences of switching an ANARI device,
not reasons to reopen the USD stage or expose device handles as public scene state.

## Device-specific roles in the first milestone

### Helide

Purpose: baseline correctness and CI-friendly smoke testing. It answers whether the
host can load a simple implementation and whether the emitted scene is accepted. Do
not optimize the UI or fixture around its image quality.

### VisRTX

Purpose: meaningful interactive GPU rendering and proof that the host is not secretly
tied to the reference device. Treat GPU/driver availability as a capability-gated test,
not a requirement for every CI worker.

### SDK debug and TSD capture

Purpose: validate API use, trace calls, and optionally capture the ANARI object state
emitted by `hdAnari`. TSD is reference/optional tooling rather than application scene
state. Neither a code trace nor final capture alone replaces semantic delta assertions.

### Island

Purpose: later measure how much renderer-specific work is needed to expose a modern
Slang/render-graph renderer through ANARI. Island assumptions must not enter the host,
`SceneDocument`, trace model, or generic viewport.

## Risks and stop conditions

| Risk | Mitigation | Stop/reconsider when |
| --- | --- | --- |
| OpenUSD/`hdAnari` API drift | Pin tested commits and hide version-specific setup in one native edge target | Required versions cannot coexist |
| Frontend/extension drift | Follow a released SDK or deliberately track `next_release`; test candidates | A required extension cannot coexist across selected devices |
| Device discovery is platform-specific | Explicit catalog configuration plus upstream loader behavior | Reliable deterministic selection cannot be achieved |
| Device switch invalidates Hydra state | Recreate delegate/render index while retaining stage/session | Switch requires application-side scene translation |
| Trace misses `hdAnari` calls | Prefer upstream layer or dispatch interception | Only host calls can be observed |
| Unsupported `hdAnari` objects | Capture call surface and report capabilities/status | Minimal fixture cannot be expressed by a target device |
| Unsafe graphics sharing | Begin with mapped color copy | Device cannot provide a safely readable frame |
| UI blocking on frame map/render | Worker/session thread and immutable frame snapshots | Device API requires unsafe UI-thread ownership |
| Slang runtime conflict with Island | Pin and test one process before device implementation | Incompatible runtimes/symbols must coexist |
| Python lifetime ambiguity | Native QObject/session ownership only | Correctness requires Python-owned handles |
| Over-generalized device kit | Source comparison and measured duplication | Only one adapter needs the proposed helper |

## Recommended first coding task

Implement only the standalone `anari_probe` spike and compatibility lock:

1. accept an explicit library candidate;
2. load it with the pinned ANARI loader;
3. route status callbacks to structured output;
4. enumerate device subtypes and extensions;
5. create and release a selected device;
6. unload the library deterministically; and
7. exercise Helide success plus failure paths.

This proves the smallest indispensable host behavior and the configured-candidate
deployment seam without coupling to USD, Hydra, Qt presentation, Island, or the
existing QRhi renderer. In parallel, prepare the smallest upstreamable `hdAnari`
selection change so a delegate receives library identity and device subtype without
process-global environment mutation. The next independently testable task is the
headless USD -> Hydra -> `hdAnari` -> Helide fixture, followed by the exact same path
through VisRTX.

Do not begin with `SceneDocument`, a Qt viewport, a custom ANARI abstraction, or an
Island adapter. Their correct shapes depend on evidence produced by these spikes and
the ANARI Device UX research deliverable.

## Explicit non-goals

Do not:

- build a new renderer;
- replace ANARI with a renderer-specific API;
- write custom USD traversal or USD-to-Island translation;
- mirror Hydra dependency state;
- hard-code Helide, VisRTX, TSD, or Island into `SceneDocument`;
- make Python own rendering;
- merge `SceneDocument` with `ShaderDocument`;
- implement complete ANARI or MaterialX support before a fixture requires it;
- force zero-copy graphics interop;
- rewrite Island to fit QRhi;
- build an Island device before studying existing devices; or
- create a device kit before ANARI-SDK helper gaps and cross-adapter duplication are
  demonstrated.

## Definition of the first architecture proof

The host architecture is proven when:

1. one USD stage remains open and authoritative;
2. one Hydra/`hdAnari` path populates Helide and VisRTX in separate device sessions;
3. switching devices requires no application-side USD traversal;
4. both frames can be displayed by one generic mapped-frame Qt viewport;
5. base-color and roughness edits are authored into a USD override;
6. the trace associates those edits with material parameter operations and recommits;
7. geometry and arrays retain identity across material-only edits;
8. `ShaderDocument` does not recompile for USD-only edits; and
9. failures and unsupported capabilities are visible through structured diagnostics.

Only then should Island be used to answer the separate research question: how little
renderer-specific code is required to add another modern renderer through ANARI?
