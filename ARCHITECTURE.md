# Miskeyed Workbench architecture

This is the concise map of the shipped 0.3.0 architecture. The canonical teaching
explanations live in [`src/docs/`](src/docs/index.rst), beginning with the
[system overview](src/docs/architecture/overview.rst).

```text
Workbench shell
├── ShaderWorkspace
│   ├── ShaderDocument(s) + DocumentSession(s)
│   ├── focused document
│   └── shared TimeContext + TimeTransport
├── WorkspaceEditor + active-document Inspector
├── tool contributions / persistent sessions
│   ├── RenderToySession  (Scene/Post bindings)
│   └── ShaderToySession  (one shader binding)
├── in-process Slang modules, compilation, reflection, and entry points
└── QRhi consumers selected by one RhiBackendPolicy
```

## Ownership boundaries

- `ShaderWorkspace` owns open documents, focus, cheap editor sessions, and shared
  evaluation objects. Opening/focusing a document does not bind a renderer.
- `ShaderDocument` owns authored source, compiler diagnostics, imports, reflection,
  resources, entry points, generated targets, and dependency identity.
- Tool sessions own runtime bindings and selected entry points. A view selector swaps
  presentation only; Render Toy and Shader Toy sessions remain alive together.
- Slang owns language semantics, module resolution, linking, reflection, entry-point
  compilation, and backend code generation. Workbench supplies packaged modules and
  project search paths at the filesystem/session edge. Value-only Workbench module
  descriptors relate host capability identity to the shader-facing contract for
  discovery; they contain no providers and do not participate in resolution.
- C++ owns application/resource lifetime, synchronization, command recording and
  submission, and Shiboken exposure. Python exposes native objects; it does not mirror
  the render core.
- Qt/QRhi owns the platform graphics abstraction. `SlangRhiWidget` and `RenderPass`
  hold consumer-side QRhi state and retire in-flight resources safely.

The open/focused/bound distinction is taught in
[Workspace and documents](src/docs/architecture/workspace.rst). Tool composition is
covered in [Tool sessions and contributions](src/docs/concepts/tool_sessions.rst).

## Data flow

```text
authored source
  → ShaderDocument
  → in-process Slang module resolution / compile
  → compiler-resolved imports compared with host/module descriptors for inspection
  → reflected entry points, parameters, resources, generated targets
  → DependencyGraph identity + dirty work
  → session-selected capabilities
  → QShader bridge / QRhi consumer
```

Hashes answer “is this product the same?”; dirty flags answer “what work must run?”
A UI-label change need not rebuild parameter layout, and a value change normally
uploads only uniform bytes. See
[Dependency identity](src/docs/architecture/dependency_graph.rst).

`TimeTransport` owns playback policy and evaluates `TimeContext`; consumers translate
that sample into host-managed Slang uniforms without changing shader identity. See
[Time is evaluation](src/docs/architecture/time.rst).

## Scope and status

Shader Toy and Render Toy are shipped. The optional ANARI host foundation and probe
are research; there is no ANARI application mode yet. Future USD/Hydra/hdAnari work
must preserve their scene-ownership boundary and remain independent of
`ShaderDocument`. See [ANARI research](src/docs/research/anari.rst).

Native placement and namespace rules are in the
[source-layout chapter](src/docs/architecture/source_layout.rst).
