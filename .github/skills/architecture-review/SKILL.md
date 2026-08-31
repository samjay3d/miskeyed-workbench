---
name: architecture-review
description: Review and change a Workbench architectural seam without guessing ownership or generalizing ahead of current needs.
---

# Architecture review

Use this checklist for changes to modules, rendering, tools, time, or future systems.
Canonical truth remains in the code and linked repository docs; this skill defines the
review process, not a second architecture description.

1. Read `AGENTS.md`, `README.md`, `ARCHITECTURE.md`, `VISION.md`, `CHANGELOG.md`, and
   `src/docs/index.rst`. Treat the checked-out implementation—not a branch name—as the
   current release baseline.
2. Read relevant public headers. Mark exported Qt/Shiboken surfaces separately from
   implementation details.
3. Trace construction, mutation, consumption, and destruction through implementations
   and the composition root. Do not infer ownership from filenames.
4. Read the contract and regression tests around the seam.
5. State current ownership: state owner, consumers, lifetime controller, language/host
   boundary, rendering boundary, and existing Python exposure.
6. State the proposed delta and why the present seam is insufficient.
7. Identify public API, ABI, Qt meta-object, and Shiboken impact explicitly.
8. Implement the smallest seam that solves the current problem. Treat future use cases
   as a replaceability check, not a request for a framework.
9. Add or update tests for ownership, identity, invalidation, lifetime, or observable
   behavior—not private layout or incidental call counts.
10. Update only the applicable docs and add notable behavior to `CHANGELOG.md` under
    `[Unreleased]`.
11. Run a consistency search for old terminology, duplicated ownership, obsolete
    assumptions, binding declarations, and capture scenario references.
12. Report deliberate non-goals and what was intentionally not generalized.

Before editing, provide a short implementation map covering steps 5–8.
