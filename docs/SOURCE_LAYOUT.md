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
│   ├── ui/                        reusable inspectors + tool-session interface
│   ├── modes/render_toy/          Render Toy runtime session
│   ├── modes/shader_toy/          Shader Toy runtime session
│   └── anari/                     optional device-neutral ANARI host API
└── src/workbench/
    ├── core/
    ├── editor/
    ├── slang/
    ├── rendering/
    ├── ui/
    ├── modes/render_toy/
    ├── modes/shader_toy/
    └── anari/
```

The existing C++ types remain in `miskeyed::workbench::slang_rhi` where that is their
established renderer API namespace; directory placement expresses responsibility and
does not silently change ABI. New device-neutral APIs use `miskeyed::workbench` or a
focused nested namespace such as `core` or `anari`.

## Placement rules

- `core` contains backend-neutral identity and temporal vocabulary: `Digest`,
  `DependencyGraph`, `TimeValue`, `TimeRange`, `TimeContext`, `TimeTransport`, and
  `ViewportCamera`. Evaluation state is separate from transport policy; core does not
  load Slang source or own QRhi objects.
- `slang` owns `ShaderDocument`, reflection/parameter models, compilation, open shader
  workspace focus/editor sessions, and the packaged Workbench Slang-module sources. It
  has no Scene/Post runtime policy.
- `rendering` owns `RenderPass` and `SlangRhiWidget`, including QRhi resource lifetime
  and deferred retirement.
- `editor` and `ui` contain reusable widgets. `WorkspaceEditor` owns tabs and binds
  reusable editor views to workspace document sessions; neither layer selects a product
  mode or runtime binding.
- `ui/WorkbenchToolFactory` is the native composition seam. Concrete tool UI sessions
  own and data-bind their contributed surfaces; the shell consumes only an interface list.
- `modes/render_toy` owns `RenderToySession`, which explicitly binds open shader documents to the active Scene/Post passes and
  presents controls for `TimeTransport`. It does not define time semantics or own
  compiler and GPU implementations. Its inspector consumes only the workspace's focused
  document; binding signals update the renderer, not inspector ownership.
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
QRhi pipeline or render target. Each successful compiler filesystem load is represented
as a module node feeding the document entry-point node. Built-in module-to-module edges
retain the authored import stack, and the inspector traverses the live graph by node ID
to show identity, content hash, dirtiness, and source. Local imported files are watched;
changes recompile their dependent document and update these nodes without replacing the
inspector with a compile-result snapshot.

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
wheel/application packaging, and loaded at the Slang session edge. `miskeyed.ui`, `miskeyed.time`, `miskeyed.viewport_camera`, and
`miskeyed.render_toy` are normal modules consumed through Slang import resolution. Project/user module locations
continue through `SessionDesc.searchPaths`. A small `ISlangFileSystem` edge exposes embedded
Workbench sources while delegating ordinary paths to the host filesystem; Slang still owns
module-name and search-path resolution.

See [Adding a Workbench tool](ADDING_TOOLS.md) for the native checklist and an unshipped
Python contribution example. This organization does not add a scene abstraction,
animation system, or compatibility facade.
