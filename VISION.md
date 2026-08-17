# Miskeyed Workbench — Vision

`miskeyed-workbench` is a **native Slang + Qt 6.8 / QRhi shader workbench** with
Shiboken6 Python bindings. This document states what the package is for and the
principles that constrain how it is built. The shipped-today design lives in
[ARCHITECTURE.md](ARCHITECTURE.md); the rules for AI agents live in
[AGENTS.md](AGENTS.md).

---

## Purpose

Edit a Slang shader and see the result immediately, through a real native runtime —
not a subprocess pipeline. Shaders are compiled in-process through Slang's
compilation API, rendered with QRhi, and driven by a live, reflection-based
parameter UI. The same native Qt objects are usable from C++ and from PySide6.

---

## Guiding principles

- **Compile in-process.** Use Slang's native compilation API; no `slangc` / `qsb`
  subprocesses at runtime.
- **Reflection is the source of truth.** Parameter UI comes from Slang reflection, not
  comment parsing or hand-maintained schemas.
- **Deterministic and incremental.** A change does the minimum work: a slider updates a
  uniform buffer, a body edit recompiles only what changed, a metadata edit rebuilds
  only the inspector.
- **Content-addressed identity.** Invalidation is tracked by a live Merkle/CAS
  dependency graph; hash identity and required work are separate concepts.
- **Clean boundaries.** Keep native systems native; integrations attach at the edges so
  the core stays portable.
- **No compatibility baggage.** Delete obsolete scaffolding rather than wrapping it.

---

## Who owns what

The GPU / system split is a hard boundary, not a suggestion.

- **Slang owns GPU work** — geometry prep, deformation/skinning, culling, GPU-driven
  draw prep, material evaluation, lighting, raster shaders, compute, post-processing.
- **C++ owns system work** — app lifecycle, device/resource creation, synchronization,
  command submission, OS/windowing, stable ownership, and Python exposure.
- **Python / PySide exposes; it does not own.** There is no Python mirror of the render
  core, and none should be added.

---

## Building blocks

We reach for these before inventing equivalents:

| Block | Role in this package |
| --- | --- |
| **Slang** | In-process shader compilation and reflection |
| **Qt / QRhi** | Portable rendering, windowing, and UI |
| **Shiboken6** | Native Qt objects exposed to PySide6 |
| **Merkle / CAS** | Identity, incremental invalidation, determinism |

---

## What success looks like

Edit Slang → dependency change detected → in-process compile → minimal QRhi state
rebuild → viewport updates — with the ownership boundaries above intact and the core
more portable, not less.
