# Miskeyed Workbench — Vision

`miskeyed-workbench` is a native technical-art workbench organized as three modes. The
first two are a **Slang + Qt 6.8 / QRhi shader workbench** with Shiboken6 Python
bindings. The third, planned mode is a neutral ANARI device workbench for real
USD/Hydra scenes. This document states what the package is for and the principles that
constrain how it is built. The shipped-today design lives in
[ARCHITECTURE.md](ARCHITECTURE.md); the rules for AI agents live in [AGENTS.md](AGENTS.md).

---

## Purpose

Give technical artists progressively larger but clearly separated experimental loops:

1. edit one Slang shader and see it immediately;
2. experiment with a small scene and post-process render pipeline; and
3. inspect a real USD/Hydra scene across interchangeable ANARI devices without turning
   Miskeyed into another renderer or scene database.

Slang shaders are compiled in-process, never through a compiler subprocess. The same
native Qt objects are usable from C++ and from PySide6.

## Product modes

### 1. Shader Toy

The smallest loop: one `ShaderDocument`, one live reflected parameter surface, and one
native viewport. It is for learning, shader experiments, and testing compiler/runtime
behavior without a scene-system dependency.

### 2. Render Toy

Shader Toy plus an explicit render pass. Today this is a Slang-authored scene pre-pass
rendered to an offscreen texture and sampled by a Slang-authored post-process pass. It
is still an experiment owned by the native Slang/QRhi runtime, not a general scene
renderer.

### 3. ANARI Device

A planned work mode for real scene and renderer work:

```text
USD/session edits -> Hydra -> hdAnari -> selected ANARI device
Slang edits       -> ShaderDocument (independent authoring path)
```

The product value is device comparison, scene-delta inspection, look overrides, and
renderer debugging in a small neutral host. Helide is a baseline, VisRTX is a useful GPU
renderer, ANARI-USD is a diagnostic/export target, and Island is a later experiment in
how easily a modern Slang renderer can be exposed as another device.

The UX for this mode must be designed from observed Hydra and ANARI behavior. It is not
defined merely as “render USD in Qt,” and Island is not the product foundation.

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

In Shader Toy and Render Toy, the GPU/system split is a hard boundary: Slang owns GPU
work; C++ owns application and QRhi lifecycle, resources, synchronization, submission,
windowing, and exposure.

In ANARI Device mode, USD owns authored meaning, Hydra owns change propagation,
`hdAnari` owns Hydra-to-ANARI translation, and the selected ANARI implementation owns
its rendering work and device resources. Miskeyed's native C++ host owns device
selection, lifecycle coordination, diagnostics, and presentation. It does not absorb
an external implementation's render core.

In every mode, Python / PySide exposes; it does not own. There is no Python mirror of a
render core, and none should be added.

---

## Building blocks

We reach for these before inventing equivalents:

| Block | Role in this package |
| --- | --- |
| **Slang** | In-process shader compilation and reflection |
| **Qt / QRhi** | Portable rendering, windowing, and UI |
| **Shiboken6** | Native Qt objects exposed to PySide6 |
| **Merkle / CAS** | Identity, incremental invalidation, determinism |
| **USD / Hydra** | Authored scene semantics and scene change propagation in ANARI Device mode |
| **ANARI / hdAnari** | Switchable renderer boundary and Hydra-to-renderer translation |

---

## What success looks like

Shader and Render Toy success remains: edit Slang → dependency change detected →
in-process compile → minimal QRhi state rebuild → viewport updates.

ANARI Device success is: keep one USD stage authoritative → observe Hydra/ANARI deltas
→ compare the same scene through interchangeable devices → author non-destructive look
overrides → keep `ShaderDocument` independent. Device installation should be an edge
deployment operation, not a rebuild of the Workbench core.
