# Native source layout

The native tree is organized by **Workbench responsibility and mode**, not by the first
renderer that shipped. Public headers and implementations use the same responsibility
folders so ownership is visible from an include path.

```text
cpp/
├── include/miskeyed/workbench/
│   ├── Export.h                   shared native export declaration
│   ├── core/                      backend-neutral identity and evaluation contracts
│   ├── editor/                    source editor and language-service API
│   ├── slang/                     Slang compilation, documents, reflection, modules
│   ├── rendering/                 QRhi pass and viewport API
│   ├── ui/                        reusable native inspectors
│   ├── modes/render_toy/          Render Toy composition root
│   └── anari/                     optional device-neutral ANARI host API
└── src/workbench/
    ├── core/
    ├── editor/
    ├── slang/
    ├── rendering/
    ├── ui/
    ├── modes/render_toy/
    └── anari/
```

The existing C++ types remain in `miskeyed::workbench::slang_rhi` where that is their
established renderer API namespace; directory placement expresses responsibility and
does not silently change ABI. New device-neutral APIs use `miskeyed::workbench` or a
focused nested namespace such as `core` or `anari`.

## Placement rules

- `core` contains only backend-neutral state and identity: `Digest`, `DependencyGraph`,
  `TimeContext`, and `ViewportCamera`. It does not load Slang source or own QRhi objects.
- `slang` owns `ShaderDocument`, reflection/parameter models, compilation, open shader
  workspace state, and the packaged Workbench Slang-module sources.
- `rendering` owns `RenderPass` and `SlangRhiWidget`, including QRhi resource lifetime
  and deferred retirement.
- `editor` and `ui` contain reusable widgets; neither selects a product mode.
- `modes/render_toy` binds open shader documents to the active Scene/Post passes and
  owns the small timeline controller. It does not own compiler or GPU implementations.
- `anari` remains an optional sibling target and never becomes a dependency of the
  shipped Slang/QRhi modes.

## Mode composition

A mode is a composition root, not a new owner of shared services:

| Mode | Composes | Does not own |
| --- | --- | --- |
| Shader Toy | one `ShaderDocument`, reflected parameters, one QRhi viewport | a scene database |
| Render Toy | open shader workspace, active Scene/Post bindings, two-pass presentation | compiler internals or general USD semantics |
| ANARI Device (planned) | USD authoring session, Hydra/hdAnari edge, selected device, neutral presentation | Hydra propagation or device render core |

`ShaderWorkspace` retains cheap authoring/compile products for open tabs. Only the
active Scene/Post bindings are consumed by `SlangRhiWidget`, so tabs do not each own a
QRhi pipeline or render target.

## Dependency direction

```text
modes  ──>  editor / ui / slang / rendering
                              slang ──> core
                          rendering ──> slang + core
                              anari ──> external ANARI loader
```

Lower layers never select a mode. `ShaderDocument` stays independent of USD, Hydra,
MaterialX, and ANARI. USD will own authored scene meaning; no parallel scene mirror
belongs in `core`. Python exposes native objects and never becomes another composition
or render core.

Workbench-owned `.slang` contracts are authored under `shaders/workbench`, embedded for
wheel/application packaging, and loaded at the Slang session edge. `miskeyed.time` is a
normal module consumed through Slang import resolution. Project/user module locations
continue through `SessionDesc.searchPaths`; a future `ISlangFileSystem` can expose
package-backed modules without changing `core` or inventing a second resolver.

This organization establishes ownership boundaries only. It does not add a speculative
mode interface, scene abstraction, animation system, or compatibility facade.
