# AGENTS.md — Miskeyed Workbench contract for AI agents

This file is the operating contract for AI coding agents (Codex, Copilot, Claude, and
any successor) working in this repository. Read it before editing. It is short on
trivia and long on **boundaries**, because boundaries are what this package is made of.
The product intent lives in [VISION.md](VISION.md); the shipped-today design lives in
[ARCHITECTURE.md](ARCHITECTURE.md).

---

## What this repo is

`miskeyed-workbench`: a native **Slang + Qt 6.8 / QRhi** shader workbench with
Shiboken6 Python bindings. Keep changes scoped to this package. Do not add
speculative infrastructure (multi-app networking, DCC adapters, external transports)
unless the current, concrete problem requires it.

---

## How to edit this repository

1. **Solve the current problem first.** Do not build speculative infrastructure.
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

---

## Ownership boundaries (do not blur)

- **Slang owns GPU work** — geometry prep, deformation/skinning, culling, GPU-driven
  draw prep, material evaluation, lighting, raster shaders, compute, post-processing.
- **C++ owns system work** — app lifecycle, device/resource creation, synchronization,
  command submission, OS/windowing, stable ownership, and Python exposure.
- **Python / PySide exposes only.** There is no Python mirror of the render core. Do
  not add one.

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
- The native extension is `_slang_qrhi.pyd` and the **C++ namespace stays `slang_qrhi`.**
  Do **not** rename the C++ internals — it is risky and buys nothing.

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

---

## What "done" looks like

Edit Slang → dependency change detected → in-process compile → minimal QRhi state
rebuild → viewport updates. Every change leaves the boundaries above intact and the
core more portable, not less.
