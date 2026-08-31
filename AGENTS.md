# AGENTS.md — Miskeyed Workbench contract for AI agents

This file is the operating contract for AI coding agents (Codex, Copilot, Claude, and
any successor) working in this repository. Read it before editing. It is short on
trivia and long on **boundaries**, because boundaries are what this package is made of.
The product intent lives in [VISION.md](VISION.md); the shipped-today design lives in
[ARCHITECTURE.md](ARCHITECTURE.md).

For active release work, the checked-out code is the source of truth. Read the current
release implementation before proposing changes; a version name or branch label is
context, not a substitute for tracing the code that is actually present.

---

## What this repo is

`miskeyed-workbench`: a native technical-art workbench with three deliberately
separate modes:

1. **Shader Toy** — the smallest live Slang shader loop;
2. **Render Toy** — live Slang authoring with a scene pass and a post-process pass; and
3. **ANARI Device** — a planned device-neutral USD/Hydra workbench for real scene and
   renderer inspection. Its UX is a design problem and is not shipped yet.

The shipped modes use Qt 6.8 / QRhi and Shiboken6 Python bindings. Keep changes scoped
to this package. Do not add speculative infrastructure (multi-app networking, custom
DCC scene extraction, external transports) unless the current, concrete problem
requires it.

---

## How to edit this repository

Before proposing architecture, read the current code, public headers, bindings, tests,
and canonical docs; trace ownership, lifetime, and call/data flow. For architectural
work use [.github/skills/architecture-review/SKILL.md](.github/skills/architecture-review/SKILL.md).

1. **Solve the current problem first.** Prefer a small coherent seam to a speculative
   framework, and preserve ownership boundaries unless the task explicitly changes one.
2. **Prefer clear boundaries over clever abstractions.**
3. **Do not create abstractions without at least one concrete reason.**
4. **Remove obsolete prototype architecture** rather than preserving compatibility.
5. **Avoid subprocesses when the relevant native / in-process API exists.**
6. **Keep native systems native.** Python / PySide *exposes*; it does not *own*.
7. **Prefer deterministic behavior.**
8. **Treat incremental invalidation and identity as first-class concerns.**
9. **Use existing standards** where appropriate rather than creating equivalents.
10. **Keep integrations at the edges.** A convenience dependency must not invert the
    architecture.
11. **Tests protect architectural invariants** — hashing, invalidation, hot reload,
    resource lifetime, deterministic outputs, API equivalence.
12. **Comment *why* a boundary exists,** not what obvious code is doing.
13. **When uncertain, optimize for** the smallest useful implementation + clean
    ownership + the ability to replace the layer later.
14. **Update the relevant docs and changelog in the same task.** Use the focused map in
    [.github/skills/docs/SKILL.md](.github/skills/docs/SKILL.md); do not touch every page
    mechanically.
15. **Review public API changes explicitly.** Headers exported by `Export.h`, Qt
    properties/invokables/signals, and `bindings/typesystem_workbench.xml` are contract
    surfaces. Shiboken exposure must be intentional and tested.
16. **Test contracts, not incidental implementation.** UI changes also require checking
    deterministic documentation capture scenarios and regenerating the focused image.
17. **Report intentional non-goals.** Every final task report must name what was
    deliberately not generalized.

---

## Ownership boundaries (do not blur)

**Shader Toy and Render Toy:**

- **Slang owns GPU work** — geometry prep, deformation/skinning, culling, GPU-driven
  draw prep, material evaluation, lighting, raster shaders, compute, post-processing.
- **C++ owns system work** — app lifecycle, device/resource creation, synchronization,
  command submission, OS/windowing, stable ownership, and Python exposure.
- **Slang owns language and module resolution.** Workbench owns packaged module bytes,
  project search-path configuration, and host/editor/runtime composition; it supplies
  these at Slang's filesystem/session edge rather than resolving imports itself.
- Host contracts and their Slang modules are related but are not the same object.
  Module metadata exists only for packaging, introspection, and user discovery: never
  turn the catalog into a service locator or second resolver, and keep host-backed
  contracts self-documenting in the Inspector and shader reference.
- **Sessions own runtime bindings and entry-point selections.** `ShaderWorkspace` owns
  documents, focus/editor sessions, and shared evaluation time. Tool contributions
  adapt sessions into views; they do not become the state owner.

**ANARI Device mode:**

- **USD owns authored scene meaning; Hydra owns scene change propagation; hdAnari owns
  Hydra-to-ANARI translation.** Do not add a parallel USD traversal or scene database.
- **The selected ANARI implementation owns its GPU rendering work and device resources.**
  The host owns selection, lifecycle coordination, diagnostics, frame presentation, and
  the authoring session—not the implementation's render core.
- **ShaderDocument remains independent.** A future ANARI implementation may consume a
  shader product through an explicit edge, but it does not own the compiler session.

**All modes:** Python / PySide exposes only. There is no Python mirror of a render core.
Do not add one.

## Do not generalize yet

Future SDF, noise, MaterialX, USD, ANARI, ML/compute, and plugin ideas are architecture
tests, not implementation requirements. Ask “does this seam leave room for that?” Do
not build a plugin manager, service locator, generalized execution graph, package
manager, or dependency-injection framework unless a current feature concretely needs
it. Do not hard-code one future consumer's assumptions into a generic layer.

## Source-of-truth map

- [`README.md`](README.md): supported user entry points and build/run overview.
- [`ARCHITECTURE.md`](ARCHITECTURE.md): concise shipped architecture; validate it against
  `cpp/{include,src}/miskeyed/workbench/{core,slang,rendering,editor,ui,modes}`.
- [`VISION.md`](VISION.md): product direction, not proof that a feature is shipped.
- [`src/docs/index.rst`](src/docs/index.rst): canonical teaching documentation.
- [`CHANGELOG.md`](CHANGELOG.md): release record. Put ordinary post-release work under
  `[Unreleased]`; when working on an active release, follow that release's stated
  context. The prepare-release skill performs the mechanical version transition.

---

## No backward-compatibility baggage

This package intentionally carries **no** compatibility layer with the old
pure-Python prototype: no `slangc`/`qsb` subprocesses, no comment-parsed parameters,
no Python-owned renderer. When you find dead prototype scaffolding, **delete it** —
do not wrap it to keep something old alive.

---

## Working facts for this repo

These are load-bearing details verified in practice. Respect them unless you have a
concrete reason and a test proving the change is safe.

**Build (editable, from the repo root):**

```powershell
# Build isolation MUST be off: shiboken6-generator comes from Qt's index, not PyPI.
# SDKs come from the environment (identical to CI) — never hardcoded paths:
$env:CMAKE_PREFIX_PATH = "<path-to>\Qt\6.8.3\msvc2022_64"   # Qt 6.8 dev SDK
$env:SLANG_ROOT        = "<path-to>\slang"                  # Slang SDK
.\.venv\Scripts\python.exe -m pip install --no-build-isolation -e . -v
```

- The base `python` on this machine is Python 2.7 — never use it. Use the venv python.
- This is Windows PowerShell: chain with `;`, never `&&`.
- SDKs are read from the environment, not the repo tree: `CMAKE_PREFIX_PATH` locates the
  Qt 6.8 dev SDK and `SLANG_ROOT` the Slang SDK (see `cmake/FindSlang.cmake`). Editable
  installs also read `SLANG_ROOT` at import time to load the Slang runtime DLLs; a
  released wheel bundles them, so it needs neither.

**Identity & naming:**

- Distribution: `miskeyed-workbench`; import: `miskeyed.workbench` (PEP 420 namespace).
- The native extension remains `_slang_qrhi.pyd`; its C++ API lives in
  `miskeyed::workbench::slang_rhi`.
- New device-neutral Workbench backend code uses `miskeyed::workbench`; do not place
  ANARI host/device infrastructure in `miskeyed::workbench::slang_rhi` merely because
  that renderer was the repository's first native target.

**Rendering:**

- The shipped backend is **Direct3D 11 + HLSL SM5.0** via FXC (`d3dcompiler_47.dll`,
  always present on Windows). D3D12/DXC is not used, so no DXC DLLs are bundled.
- Never `reset()`/free a QRhi resource synchronously in `render()`/`initialize()` while
  a prior frame may still reference it — retire it through the deferred-release
  graveyard for `>= maxFrameLatency + 1` frames. Synchronous frees crash the D3D11
  backend (use-after-free, `0xC0000005`).

**Reflection & invalidation:**

- Uniform globals are wrapped in an implicit constant buffer — unwrap
  `getElementTypeLayout()` before reading fields, or reflection returns zero params.
- The uniform buffer is packed by each parameter's explicit `offset` (memcpy), not by
  list order, so descriptor order is UI-only and safe to reorder/group.
- Hash identity and required work are separate concepts. A source edit does not make
  every product a hash-child of source; compiler outputs are independent semantic nodes.

**Shiboken gotchas:**

- `WorkbenchWindow.h` must `#include` `LspClient.h` (not forward-declare) because of
  `QList<LspDiagnostic>` as a member.
- Keep the `SlangRhiWidget` mouse/wheel event overrides **private** — Shiboken skips
  private members; making them protected breaks binding generation.

**Testing native windows headless:**

- Only construct/exercise `WorkbenchWindow` inside a running event loop with a
  `QTimer` quit. Building one headless without `app.exec()` throws a benign teardown
  AV (slangd/QRhi with no loop) that is a harness artifact, not a real bug.

**Reading release health:**

- Package, fresh installation, installed-package contracts, native contracts, and QRhi
  runtime smoke are separate evidence levels. Support claims require a real passing lane
  at the claimed level; never infer rendering from a built wheel or successful import.
- Installed-wheel contracts use public APIs and packaged resources only. Repository-local
  fixtures and implementation checks remain source-tree tests. Platform display/driver
  setup belongs in workflows, and runtime validation uses the canonical
  `miskeyed-workbench --rhi <backend> --rhi-smoke-test` harness.

---

## What "done" looks like

For Shader Toy and Render Toy: edit Slang → dependency change detected → in-process
compile → minimal QRhi state rebuild → viewport updates.

For the planned ANARI Device mode: edit USD → Hydra/hdAnari emits the minimum ANARI
delta → the selected device updates → the neutral viewport and diagnostics update.
Changing devices may rebuild device-side state but must not change USD ownership or
merge scene state with `ShaderDocument`.
