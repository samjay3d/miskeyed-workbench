# Miskeyed ANARI host implementation plan

## Status and evidence boundary

This is the implementation plan for making Miskeyed a device-neutral ANARI host and
technical-art workbench. Island is a future device-adapter experiment, not the product
foundation and not a prerequisite for the host.

The plan is grounded in the current Miskeyed source and the public ANARI API surface.
The execution environment used to prepare it could not fetch GitHub sources, so exact
upstream versions and implementation-file citations for ANARI-SDK, `hdAnari`, Helide,
VisRTX, ANARI-USD, and Island remain mandatory outputs of the first source-audit spike.
They are recorded as unresolved rather than guessed.

No production integration should begin until that spike pins mutually compatible
commits and verifies the API assumptions below against upstream source.

## Product statement

Miskeyed should give technical artists a neutral environment in which they can:

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
observable and shader-authoring state remains independent.

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
       Helide              VisRTX              ANARI-USD
      baseline           GPU renderer          diagnostics

                              future
                                |
                                v
                      Island ANARI device
                                |
                                v
                         Island + Slang
```

The line from `ShaderDocument` to the host denotes future consumption of compiled
shader products by a capable device. It does not grant the host or `SceneDocument`
ownership of Slang compiler state.

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
2. Helide, VisRTX, and ANARI-USD can use the same ANARI ABI/version.
3. Switching the device used by `hdAnari` can repopulate renderer state without
   reopening the USD stage in application code.
4. `hdAnari` exposes enough hooks to associate Hydra dirty events with captured ANARI
   operations.
5. VisRTX supports the Windows/toolchain target used by the Workbench.
6. ANARI-USD is a loadable device compatible with the chosen SDK rather than a tool
   requiring a different integration path.
7. A mapped color channel has an image format and row layout that the generic Qt
   presenter can handle across all selected devices.
8. A future Island device can coexist with the Workbench Slang runtime.

These are spike questions, not facts to build around.

## Upstream compatibility matrix to pin

The first research deliverable is a checked-in lock table populated from actual
upstream commits and build output:

| Component | Current requirement | Evidence required | Stop condition |
| --- | --- | --- | --- |
| Qt/PySide/Shiboken | 6.8.x in this package | Current CMake and wheel build | Host requires an incompatible event/UI toolchain |
| Slang | Environment-provided SDK; current compiler code uses native API | Exact installed version and Island requirement | Two incompatible Slang runtimes must load in one process |
| OpenUSD | Unpinned | Commit/release exporting required Hydra host APIs | `hdAnari` requires an incompatible Hydra generation |
| ANARI-SDK | Unpinned | Header ABI, loader, helper and Helide targets | Selected devices do not share an ABI |
| `hdAnari` | Unpinned | Repository, target, compatible OpenUSD and ANARI commits | Cannot inject/select the intended ANARI library/device |
| Helide | Expected from ANARI-SDK; verify | Target and runtime library name | Cannot load through the same host API |
| VisRTX | Unpinned and optional | Supported OS/GPU/toolchain and ANARI ABI | Requires an unavailable GPU stack for baseline CI |
| ANARI-USD | Unpinned and optional | Repository, device name, output semantics | Incompatible or unavailable; tracing must still work without it |
| Island | Future research only | Commit, Slang/Vulkan requirements, extension seams | Requires host layers to depend on Island internals |

Store the resulting matrix in `docs/anari/COMPATIBILITY.md` with exact repository URLs,
commits, CMake targets, loadable library/device names, extension lists, and runtime
DLLs. CI may exercise only Helide initially; VisRTX and ANARI-USD can be capability-
gated.

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

The source audit must verify every signature and availability against the pinned
`<anari/anari.h>` and extension headers.

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

1. an existing ANARI validation/debug/tracing layer in ANARI-SDK;
2. ANARI-USD's diagnostic/export capability;
3. `hdAnari` hooks or logging;
4. only then, a small dispatch interposer owned by Miskeyed.

Do not permanently fork `hdAnari` merely to log calls. The chosen mechanism must see
the calls made by `hdAnari`, not just calls made by the host around it.

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

| Responsibility | ANARI-SDK helpers | Helide | VisRTX | ANARI-USD | Renderer-specific? |
| --- | --- | --- | --- | --- | --- |
| ABI dispatch/export | TBD | TBD | TBD | TBD | Usually generic; verify |
| Device/object handles | TBD | TBD | TBD | TBD | Usually generic; verify |
| Reference counting | TBD | TBD | TBD | TBD | Generic candidate |
| Parameter storage/typing | TBD | TBD | TBD | TBD | Generic candidate |
| Commit semantics | TBD | TBD | TBD | TBD | Split; verify |
| Array ownership/deleters | TBD | TBD | TBD | TBD | Generic candidate |
| Status callbacks | TBD | TBD | TBD | TBD | Generic candidate |
| Extension reporting | TBD | TBD | TBD | TBD | Split; verify |
| Geometry/material creation | N/A or helper hooks | TBD | TBD | diagnostic representation | Renderer-specific |
| Render/frame channels | N/A or helper hooks | TBD | TBD | export/inspection | Renderer-specific |

Each `TBD` must be replaced with repository, commit, path, symbol, responsibility, and
license. This evidence determines whether a device kit is justified.

## File-level implementation plan

Names below are proposed and should be adjusted only when pinned upstream APIs provide
a concrete reason.

### Stage 0: compatibility and API spike

Create:

- `docs/anari/COMPATIBILITY.md` — pinned upstream commits, targets, runtime artifacts,
  extension lists, and supported configurations.
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

- `cmake/FindANARI.cmake` only if upstream does not export usable CMake config targets;
- `cpp/include/slang_qrhi/AnariLibrary.h`;
- `cpp/src/AnariLibrary.cpp`;
- `cpp/include/slang_qrhi/AnariDeviceCatalogModel.h`;
- `cpp/src/AnariDeviceCatalogModel.cpp`;
- `tests/test_anari_library.cpp`.

Modify:

- `CMakeLists.txt` — add `SLANG_QRHI_WITH_ANARI`, an optional native target, and tests;
- binding files only after the C++ ownership API stabilizes.

Ownership: `AnariLibrary` owns the library handle; a device session retains its
library. The catalog owns probe results, not active render devices.

Milestone: the native app can enumerate configured Helide and VisRTX candidates and
show probe failures without loading USD or creating a viewport.

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

### Stage 5: optional ANARI-USD diagnostics

Modify:

- `docs/anari/COMPATIBILITY.md` with the backend's exact output and limitations;
- device catalog configuration to make ANARI-USD discoverable when installed.

Create tests or golden artifacts only if ANARI-USD output is deterministic enough.
Keep it optional and do not make ordinary host startup depend on it.

Milestone: the same `hdAnari` scene can be inspected/exported through ANARI-USD, or a
documented incompatibility justifies relying on the trace layer instead.

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
VisRTX, ANARI-USD, and ANARI-SDK helpers, separating ABI/object boilerplate from
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

### ANARI-USD

Purpose: inspect or export the ANARI state emitted by `hdAnari`. Treat it as an optional
diagnostic device. It complements but does not replace ordered operation tracing,
because an exported final state alone may not prove incremental object identity.

### Island

Purpose: later measure how much renderer-specific work is needed to expose a modern
Slang/render-graph renderer through ANARI. Island assumptions must not enter the host,
`SceneDocument`, trace model, or generic viewport.

## Risks and stop conditions

| Risk | Mitigation | Stop/reconsider when |
| --- | --- | --- |
| OpenUSD/`hdAnari` API drift | Pin tested commits and hide version-specific setup in one native edge target | Required versions cannot coexist |
| ANARI ABI mismatch | Load all first devices through one pinned loader/header set | Helide and VisRTX require incompatible ABIs |
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

Implement only the standalone `anari_probe` spike after completing the upstream source
audit and compatibility lock:

1. accept an explicit library candidate;
2. load it with the pinned ANARI loader;
3. route status callbacks to structured output;
4. enumerate device subtypes and extensions;
5. create and release a selected device;
6. unload the library deterministically; and
7. exercise Helide success plus failure paths.

This proves the smallest indispensable host behavior without coupling to USD, Hydra,
Qt presentation, Island, or the existing QRhi renderer. The next independently
testable task is the headless USD -> Hydra -> `hdAnari` -> Helide fixture, followed by
the exact same path through VisRTX.

Do not begin with `SceneDocument`, a Qt viewport, a custom ANARI abstraction, or an
Island adapter. Their correct shapes depend on evidence produced by these two spikes.

## Explicit non-goals

Do not:

- build a new renderer;
- replace ANARI with a renderer-specific API;
- write custom USD traversal or USD-to-Island translation;
- mirror Hydra dependency state;
- hard-code Helide, VisRTX, ANARI-USD, or Island into `SceneDocument`;
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
