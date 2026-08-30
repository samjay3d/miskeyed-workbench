---
name: docs
description: Select and validate the documentation surfaces required by a Workbench change without mechanically editing every document.
---

# Documentation update checklist

Map the actual change to its teaching surface:

| Change | Update |
|---|---|
| Major user workflow or product entry point | `README.md`, then focused `src/docs/using/` pages |
| Mental model or terminology | `src/docs/concepts/` |
| Ownership, lifetime, or architecture | `ARCHITECTURE.md` and focused `src/docs/architecture/` pages |
| Slang language/module/entry-point contract | `src/docs/workbench_slang/` and applicable `src/docs/reference/` pages |
| Public CLI, environment, backend, or build option | `src/docs/reference/` and README when it affects onboarding |
| Contributor/release process | `AGENTS.md`, `.github/skills/`, and `src/docs/contributing/` as applicable |
| Notable release behavior | `CHANGELOG.md` under `[Unreleased]` |
| Visible UI/layout/state | relevant capture scenario plus one focused regenerated screenshot |

Do not mechanically touch every document. Search for stale terms and cross-links after
editing. Build docs with warnings as errors using the command in
`src/docs/contributing/index.rst`.

Use `[Unreleased]` for ordinary post-release work. If the task is explicitly part of an
active release, update that release record instead of enforcing a hard-coded version
policy in this generic checklist.
