# Native source layout

The source tree is organized around **Workbench responsibilities and product modes**,
not around whichever backend happened to ship first. Slang is an important authoring
and rendering component; it is not the application shell or the owner of future scene
workflows.

```text
cpp/
├── include/miskeyed/workbench/
│   ├── slang_rhi/                 shipped public API (and Shiboken surface)
│   └── anari/                     experimental ANARI host API
└── src/workbench/
    ├── core/                      shared contracts, pass state, invalidation
    ├── editor/                    reusable editor and language-service UI
    ├── slang/                     compilation, reflection, shader documents
    ├── rendering/                 QRhi presentation and resource lifecycle
    ├── ui/                        reusable native inspector widgets
    ├── modes/
    │   └── render_toy/            current two-pass authoring composition
    └── anari/                     optional device discovery and lifecycle edge
```

Public headers remain under `slang_rhi` because that is the established API and native
extension boundary. The implementation layout is intentionally more granular: a
public API name must not force every future Workbench subsystem into the same module.
New device-neutral APIs go directly under `miskeyed/workbench` and use the
`miskeyed::workbench` namespace (with a focused nested namespace where useful).

## Mode composition

A mode is a composition root, not a new owner of compiler, editor, or renderer state:

| Mode | Composes | Does not own |
| --- | --- | --- |
| Shader Toy | one `ShaderDocument`, reflected parameters, one QRhi viewport | a scene database |
| Render Toy | scene and post-process `ShaderDocument`s, two-pass QRhi presentation | general USD scene semantics |
| ANARI Device (planned) | USD authoring session, Hydra/hdAnari edge, selected device, neutral presentation | Hydra change propagation or the device render core |

The current `WorkbenchWindow` implementation lives in `modes/render_toy` because its
two documents and scene-to-post-process connection are concretely Render Toy behavior.
Shader Toy can later receive its own small composition root without conditionalizing
the compiler or viewport. ANARI Device must similarly be added as a sibling mode only
after the USD/MaterialX authoring-session boundary is designed and tested.

## Dependency direction

```text
modes  ──>  editor / ui / authoring backends / presentation
                  slang ──> core
                  rendering ──> slang products
                  anari ──> external ANARI loader
```

Lower layers never select a mode. In particular:

- `ShaderDocument` remains independent of USD, Hydra, MaterialX, and ANARI.
- USD will own authored scene meaning; no Workbench scene mirror belongs in `core`.
- MaterialX support will attach to the USD authoring lane rather than being translated
  into Slang UI metadata.
- ANARI stays optional and cannot become a dependency of Shader Toy or Render Toy.
- Python exposes native objects and never becomes a second composition or render core.

Render Toy's pass-local QRhi state is represented by `RenderPass` instead of being
buried in the viewport implementation. Its target composition remains in the viewport,
where QRhi resource lifetime is owned. The shared `ViewportCamera` contract likewise
defines the host field names and emits the visible `WorkbenchViewportCamera` Slang
header used by built-in scene sources. Camera controls are therefore reflected authored
data, not magic uniforms injected by the compiler.

Workbench-owned Slang declarations live as source files in `shaders/workbench`, are
embedded into the native target, and are consumed through one `WorkbenchHeaders`
catalog. The compiler and the read-only **Headers** panel use that same catalog, making
UI attributes and viewport bindings inspectable without duplicating their definitions.

This layout is deliberately only a reorganization of responsibilities. It introduces
no speculative mode interface, scene abstraction, or compatibility facade before
there is a concrete second implementation that needs one.
