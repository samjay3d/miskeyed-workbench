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

### Render Toy documents and views

`ShaderWorkspace` owns open authoring documents, the focused tab, and the active Scene
and Post bindings. Each `ShaderDocument` retains source, dependency-graph identity,
reflection, diagnostics, and generated targets. `SlangRhiWidget` consumes only the two
active bindings: it does not own the tab list, and inactive tabs do not allocate QRhi
pipelines or textures. Source, generated output, and compare are views of the focused
document, while reflection remains an inspector concern.

### Time and deterministic evaluation

`core::TimeContext` is host evaluation state shared by the active Render Toy scene and
post passes. Controllers such as the timeline advance discrete frames; a sample is
always derived as `time = frame / frameRate` and `deltaTime = 1 / frameRate`. Updating
time writes host-managed reflected uniform bytes and schedules rendering without
changing shader, module, or dependency hashes. Future evaluated-product identities can
therefore combine dependency, parameter, resource, and time-sample identities without
making time part of authored source identity.

The packaged `miskeyed.time` Slang module supplies `workbenchTime`; the compiler imports
that standard host contract automatically. Built-in module registration is an edge of
the Slang session, while project and user imports continue to use session search paths.

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
