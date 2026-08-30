# Miskeyed Workbench — Vision

## Purpose

Workbench explores how a native technical-art application can make shader and tool
iteration immediate without collapsing authoring, evaluation, compiler, UI, and
rendering ownership into one object. It should be useful to technical artists and
also legible to tools and graphics engineers learning modern DCC architecture.

## Direction

The near-term product is a small shell in which Shader Toy and Render Toy demonstrate
coexisting tool sessions over shared documents, reflection, time, and QRhi services.
Longer-term work may add Lookdev, rendering, USD/MaterialX, and device-neutral ANARI
workflows, but research does not become shipped architecture until its ownership and
runtime evidence are proven.

## Principles

1. **Documents are authored state; bindings are runtime policy.** A tool consumes a
   document without owning it.
2. **Evaluation is explicit and deterministic.** Time is a typed sample, not a side
   effect of a playback widget.
3. **Language semantics stay with Slang.** Workbench integrates modules, reflection,
   and compiler products rather than inventing a parallel shader language.
4. **Identity and required work stay separate.** Content hashes establish sameness;
   dirty state schedules the minimum useful update.
5. **Native systems remain native.** C++/Qt/QRhi own lifecycle and execution. Python
   exposes the same objects without becoming a second render core.
6. **Tools contribute to a shell.** View selection is presentation, not session
   ownership or application mode switching.
7. **Portability claims follow evidence.** Package, construction, and runtime-render
   validation are distinct.
8. **Research stays at the edges.** USD owns authored scene meaning, Hydra owns change
   propagation, hdAnari owns translation, and ANARI devices own their render cores.

## Success

Editing an authored dependency should produce an in-process compile, the minimum
semantic invalidation, and an updated native viewport. A contributor should be able
to understand why each update occurs, replace one layer without taking over its
neighbors, and extend the shell without making an existing tool the application.

Current implementation belongs in [ARCHITECTURE.md](ARCHITECTURE.md); the detailed
learning path begins at [src/docs/index.rst](src/docs/index.rst).
