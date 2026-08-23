# Omniverse Kit integration boundary

## Finding

Kit can be evaluated as an optional Python host without introducing a Kit dependency in
Workbench's native targets. The first spike therefore uses the existing Shiboken module. It
does **not** prove binary compatibility with every Kit release; that requires running the
optional test with a Kit build whose Python ABI can load the Workbench wheel.

If that runtime test fails and cannot be solved by packaging an ABI-compatible wheel, stop.
Do not link Carbonite or the Kit SDK into `slang_qrhi_core` as a workaround.

```text
                         host-neutral ownership
  +----------------+      +-------------------------+
  | Qt application |----->| slang_qrhi_core         |
  | WorkbenchWindow|      | Slang + invalidation    |
  +----------------+      | QRhi standalone renderer|
                          +------------^------------+
                                       |
  +----------------------+             | Shiboken API
  | Omniverse Kit        |             |
  | extension manager/UI |---> Kit adapter (Python)
  +----------------------+      integrations/kit only
```

There is no dependency arrow from the native core to Kit.

## Inventory

The current CMake boundary is coarser than the conceptual boundary: one static target includes
both authoring/system objects and Widgets UI. Consequently, a **C++ consumer of the library**
links Qt Core, Gui, and Widgets even when it does not construct UI. This is technical debt, but
it does not require a restructure for the Python spike.

| Area | Objects | Runtime requirement | Classification |
|---|---|---|---|
| Identity/invalidation | `Digest`, `DependencyGraph` | Qt Core object model; no widget | host-neutral system API |
| Compilation/reflection | `SlangCompiler`, `ShaderDocument`, `ShaderParameterModel` | Qt Core/Gui types and Slang; a `QCoreApplication` event loop is useful for live debounce, but explicit `compile()` is synchronous | host-neutral authoring/backend API |
| Parameter UI | `ParameterInspector` | `QWidget`/`QApplication` | standalone UI |
| Rendering | `SlangRhiWidget` | `QWidget`, QRhi lifecycle, graphics device | standalone renderer/viewport |
| Editor/application | `CodeEditor`, `ShaderHighlighter`, `LspClient`, `WorkbenchWindow` | Widgets; window lifecycle (and process management for LSP) | standalone application |

No ANARI or USD/Hydra target exists on this branch yet. Those items remain intended seams, not
APIs that this adapter can honestly claim to consume.

### Candidate public host APIs

* `DependencyGraph.h` is usable for native lifecycle and invalidation checks without a widget.
* `ShaderDocument.h` is the current authoring facade: load/save, explicit compilation,
  diagnostics, generated targets, parameters, and the dependency graph.
* `ShaderParameterModel.h` exposes reflected rows and value edits. Its Qt model surface is
  host-neutral in behavior but awkward for non-Qt hosts; a future immutable reflection snapshot
  is justified only if the spike demonstrates that this impedance matters.
* `SlangCompiler.h` is native and UI-free, but is not wrapped by Shiboken. The adapter should not
  create a duplicate wrapper while `ShaderDocument` already delegates to it.

`SlangRhiWidget.h` and `WorkbenchWindow.h` are explicitly not host-facing Kit contracts.

## Binding sufficiency and smallest proof

The bindings already expose `DependencyGraph`, `ShaderDocument`, `ShaderParameterModel`, and
their invokable operations. That is sufficient for both the lifecycle smoke call and a first
Slang inspector. A Python Kit extension is therefore smaller than a direct C++ extension and
reuses the wheel's existing native ABI boundary.

The core adapter constructs `DependencyGraph`, calls `ensureNode()`/`nodeCount()`, reports the
installed distribution version, and releases the object at shutdown. The Slang tool constructs
`ShaderDocument`, calls its real load/compile/reflection/value APIs, and never invokes the
standalone viewport.

## Runtime validation gate

1. Install an ABI-compatible `miskeyed-workbench` wheel into the Python environment visible to
   the selected Kit application (do not make the extension silently install packages at startup).
2. Add `integrations/kit/exts` to Kit's extension search paths.
3. Enable `miskeyed.workbench.core`; verify the native node call in the log.
4. Disable and re-enable it with `integrations/kit/kit_lifecycle_smoke.py`.
5. Enable `miskeyed.workbench.slang_tools`, load `shaders/default.slang`, compile, edit a
   reflected value, and verify no compile is triggered by that value edit.

NVIDIA documents the Python extension lifecycle through `omni.ext.IExt`, extension metadata and
dependencies in `extension.toml`, and extension search paths in the Kit manual:
[Python extension example](https://docs.omniverse.nvidia.com/kit/docs/kit-manual/latest/guide/extensions_advanced.html),
[extension configuration reference](https://docs.omniverse.nvidia.com/kit/docs/kit-manual/latest/guide/extensions_advanced.html#extension-config-toml).

## Stop conditions

Stop or reduce this integration if Kit requires linking the core against Kit/Carbonite, if the
wheel cannot coexist with Kit's Python/Qt runtime, or if useful scene integration requires a
second owner of USD state. A failed Python ABI experiment can justify a *thin C ABI adapter at
the integration edge*; it does not justify putting host lifecycle in the renderer.
