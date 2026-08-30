# Miskeyed Workbench architecture

`miskeyed-workbench` is a native technical-art application and SDK. Slang/QRhi is one
shipped backend, not the product boundary. The old Python renderer/toolchain API is
intentionally not part of this package.

This document describes the shipped **Shader Toy** and **Render Toy** modes. The
planned **ANARI Device** mode is not shipped architecture; its product boundary and
research gates live in [docs/ANARI_HOST_IMPLEMENTATION_PLAN.md](docs/ANARI_HOST_IMPLEMENTATION_PLAN.md).

The renderer lives in `miskeyed::workbench::slang_rhi`; device-neutral backend code
lives beside it in `miskeyed::workbench`. The nested renderer namespace keeps its API
distinct without presenting ANARI host infrastructure as part of the Slang/QRhi layer.

The CMake project is the `miskeyed_workbench` product umbrella. Consumers inside the
build use the namespaced aliases `miskeyed::workbench::slang_rhi` and, when ANARI is
enabled, `miskeyed::workbench::anari_backend`. These target names make the product
boundary explicit. The private `_workbench` extension name remains backend-specific,
while its generated types use the real nested C++ namespace. A compile test includes
and links both native surfaces to keep that boundary building before ANARI host work
expands it.

Implementation files are grouped by Workbench responsibility and mode rather than in
a flat renderer-owned directory. See [the native source layout](docs/SOURCE_LAYOUT.md)
for the dependency direction and the intended placement of the USD/MaterialX authoring
lane before ANARI Device UI work begins.

## Ownership

- **Qt 6.8 / QRhi** owns the graphics device, render targets, command buffers, and widget lifecycle.
- **Slang C++ API** owns source compilation, module composition, linking, reflection, entry-point hashes, and backend shader generation.
- **QShader bridge** packages in-memory Slang output for QRhi. There are no runtime compiler subprocesses.
- **ShaderDocument** is the live authoring state and connects source, reflection, parameters, dependency state, and generated shader packages.
- **ShaderParameterModel** is the authoritative Qt model for reflected values. It backs both native Qt and Shiboken/PySide consumers.
- **DependencyGraph** is a shader-specific persistent Merkle DAG. Hashes answer identity; dirty flags answer work scheduling.
- **SlangRhiWidget** is the embeddable native viewport used by both the standalone executable and PySide6.

### Workspace documents and tool sessions

`ShaderWorkspace` owns open authoring documents, exactly one focused document, and cheap
per-document editor sessions (cursor, selection, scroll, view mode, and generated target).
It does not infer or mutate runtime bindings when a document opens. It also owns the
shared `TimeContext` and `TimeTransport`. `RenderToySession` separately owns the active
Scene/Post bindings; `ShaderToySession` owns one fullscreen-shader binding. These sessions
are contributions rather than exclusive application modes: both can bind the same
`ShaderDocument` and remain alive at the same time. Each `ShaderDocument`
retains source, dependency-graph identity, reflection, diagnostics, and generated
targets. `SlangRhiWidget` is the QRhi render-provider/presentation seam. A Render Toy
instance consumes Scene/Post bindings while a Shader Toy instance consumes one document
with no scene pre-pass; neither owns the tab list, and inactive tabs do not allocate QRhi
pipelines or textures. Source, generated output, and compare are views of the focused
document, while reflection remains an inspector concern. The structured **Dependencies**
view presents the focused shader and compiler-resolved imports as the default hierarchy,
including paths, hashes, and dirty state. Selecting a source/module resolves its current
payload from the live `DependencyGraph`, while compiler/renderer products are available
under a collapsed Advanced graph branch. Resolved project files are watched, so editing
an imported module recompiles the document and updates the same graph and inspector in
place.

`WorkspaceEditor` is the reusable document-editor group. It owns tabs, close/reorder and
keyboard navigation, the reusable authored/generated editor widgets, view selection, and
binding those widgets to `DocumentSession`. It observes `ShaderWorkspace`; it has no
Render Toy slot or GPU ownership. `WorkbenchWindow` composes this editor group with the
coexisting sessions, viewports, inspector, and transport. A persistent tool selector swaps
only a stacked contribution surface: Render Toy contributes its Scene/Post split and local
binding actions, while Shader Toy contributes one larger viewport and its local binding
action. The editor, inspector, timeline, toolbar, status bar, focus, and session objects are
not rebuilt or rebound by this switch. The selector is populated from registered tool
contributions rather than an application-wide mode enum or switch. A studio integration can
register another titled `QWidget` contribution surface and keep its own session state at that
edge; unregistering it removes only that contribution and activates the next registered tool.
The shipped-tool composition lives in `WorkbenchToolFactory`, not in `WorkbenchWindow`:
mode adapters implement `WorkbenchToolUiSession`, build and data-bind their own surface, and
the shell only enumerates the resulting interface list. A studio can provide sessions that
implement the same interface from its own composition root.

The right-hand inspector is a single active-document surface: Parameters, Resources,
Dependencies, and Compilation all switch when workspace focus changes. Viewport clicks
change workspace focus to the document currently bound to that pass; Render Toy binding
changes never directly select an inspector model. This keeps focused document, editor,
and inspector as one state while Scene/Post remain separate runtime bindings. The
inspector spans the viewport and editor region, and the shared transport sits at the
bottom of the document workspace as a controller of Render Toy evaluation rather than a
property of any shader tab.

Resources are compiler-reflected textures, samplers, buffers, and parameter blocks with
binding/space information. Dependencies default to the authored shader/module stack;
compiler and renderer DAG products remain collapsed under an Advanced graph branch.
Compilation presents status, elapsed compile time, entry points, generated targets,
profile, and diagnostics as structured document information.

### Time and deterministic evaluation

`core::TimeValue` carries a floating-point coordinate and its units-per-second rate;
`core::TimeRange` describes a domain without clamping samples. `core::TimeContext` holds
only the current value, delta, and a monotonic evaluation index. It has no playback
range, playing state, stepping, or looping behavior.

`core::TimeTransport` is the replaceable playback controller used by the Workbench's small
timeline. It owns range and loop policy and drives the workspace context consumed by all
active tool sessions. USD, OTIO, an offline renderer, or another host can instead set the same
context directly without translating through an integer frame. Context changes upload
host-managed uniform bytes without changing shader or dependency identity.
Rate changes rescale current/range coordinates to preserve seconds. Realtime controllers
advance it with elapsed monotonic seconds, while explicit seek/step remains deterministic.

The packaged `miskeyed.ui`, `miskeyed.time`, and `miskeyed.shader_toy` modules provide host contracts. Authored
shaders explicitly import the contracts they use; Render Toy samples import `miskeyed.viewport_camera` and
`miskeyed.render_toy` according to their role. An `ISlangFileSystem` edge supplies these
packaged modules to the Slang session, while project and user imports continue through
Slang session search paths. Slang—not a Workbench header catalog—owns module resolution.

## Merkle DAG rule

A source edit does not make every semantic product a hash-child of source. Compiler outputs become independent semantic nodes after compilation.

For example, an implementation-only source edit may change `EntryPoint` while `ParameterLayout` retains the same digest. The viewport then rebuilds the shader/pipeline while the parameter inspector remains untouched.

```text
Source -> EntryPoint ---------------------> Pipeline
                 ^                           ^
                 |                           |
          Slang compile                ParameterLayout
                                            |      |
                                            v      v
                                        UiSchema  Values
```

This is deliberately different from a serialized snapshot Merkle tree: the graph is long-lived, shared dependencies form a DAG, dependency ordering is canonicalized, and invalidation semantics are node-kind specific.

## Runtime update classes

- parameter value: uniform buffer update only
- UI metadata: inspector/model update only
- texture/resource content: resource upload only
- reflected binding layout: bindings + pipeline rebuild
- shader implementation: compile + shader package + pipeline rebuild
- render state: pipeline rebuild only

The target UX is Substance/Houdini-like: edit shader declarations, let reflection generate controls, and keep ordinary control changes out of the compiler path.
