# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- A document-oriented workspace and reusable `WorkspaceEditor`. Scene and post-process
  shaders can remain open in closeable/reorderable tabs; the focused document drives
  editor, inspector, diagnostics, dependency, and compilation context. Per-document
  cursor, selection, scroll, generated target, and view-mode state survive tab switches.
- Source, generated, and side-by-side compare views for the focused shader document,
  so generated code is available for inspection without permanently occupying a third
  editor column.
- Backend-neutral `TimeValue`, `TimeRange`, and `TimeContext` primitives plus a separate
  `TimeTransport`. Floating-point/subframe samples carry their rate, rate conversion
  preserves seconds, and playback policy no longer lives in evaluation state.
- A minimal Render Toy transport bar with play/pause, deterministic stepping, scrubbing,
  playback range, and frame-rate controls. Elapsed-time playback and explicit sample
  evaluation both drive one context shared by the active Scene and Post passes.
- A packaged `miskeyed.time` Slang module. Its host-managed `time`, `deltaTime`, `frame`,
  and `frameRate` fields update through reflected uniforms without recompiling source.
- Shared native `ViewportCamera` and `RenderPass` contracts. Camera parameter names are
  centralized, while pass-local QRhi buffers, bindings, pipelines, and dirty state no
  longer live directly on the viewport widget.
- Authored Workbench Slang contracts for reflected UI attributes, viewport camera data,
  Render Toy pass data, and time, embedded for installed applications and wheels and
  resolved as ordinary `miskeyed.*` imports through Slang's filesystem/session APIs.
- Live dependency inspection for the focused shader. The primary tree shows resolved
  imports, paths, content hashes, source, and dirty state; the internal compiler/render
  DAG remains available as a progressively disclosed Advanced view. Imported project
  files are watched and recompile their dependent document in place.
- Structured active-document inspector views for reflected parameters and resources,
  import dependencies, entry points, generated targets, compile status/timing, and
  diagnostics.
- Authored, packaged Render Toy scene and post-process samples, including animated
  examples that exercise the shared time contract and restoreable default samples.
- Native contract tests for viewport-camera and pass bindings, workspace focus/session
  behavior, module dependencies, and deterministic subframe/time-transport evaluation.
- CI capture of a canonical native Workbench window image, published as the
  `workbench-documentation` artifact for review.
- Read-only wheel and source-distribution artifacts for pull requests targeting a
  `release/*` branch, using the same reusable distribution build as release publishing
  without granting review builds OIDC or repository-write permissions.
- An opt-in, developer-facing ANARI backend foundation. With
  `MISKEYED_WORKBENCH_WITH_ANARI=ON`, native Workbench code can load configured ANARI
  libraries, enumerate device subtypes and extensions, collect status diagnostics, and
  create device sessions with deterministic library/device teardown. The feature
  remains off by default and is not exposed in the application UI yet.
- A standalone `workbench-anari-probe` build under `spikes/anari_probe` for exercising
  ANARI discovery and device lifecycle without Qt, Slang, Shiboken, or the Workbench
  application target.
- ANARI backend CI coverage using a pinned ANARI-SDK revision, candidate parser tests,
  and a load/query/create/commit/release smoke test against the SDK sink device.

### Changed

- Reorganized native implementation and public headers by responsibility: core,
  editor, Slang, rendering, reusable UI, mode composition, and ANARI integration.
  Render Toy is now a composition root rather than the owner of shared services.
- Separated generic workspace state from Render Toy runtime policy. Opening a document
  only opens and focuses it; `RenderToySession` explicitly owns Scene/Post bindings and
  the transport, while only the bound pair feeds live QRhi state.
- Reworked the Render Toy UI around open shader documents and explicit Scene/Post
  viewport bindings. Clicking a viewport focuses its bound document, while selecting a
  tab reliably updates the editor and every active-document inspector section.
- Moved host time updates through the existing reflected global-parameter upload path.
  Time changes update active GPU evaluation state without changing shader or dependency
  identity and without triggering Slang compilation.
- Marked Workbench-provided time fields as host-managed so they do not appear as
  authored parameter controls or collide with user uniforms that have common names.
- Replaced the first-class Workbench Headers browser and concatenated shader contracts
  with explicit Slang imports. Project/user locations remain session search paths, and
  the small packaged-module filesystem seam can later be replaced without introducing
  a second package resolver.
- Updated the native CMake source registration, install layout, Shiboken declarations,
  README, and architecture documentation to match the responsibility-oriented tree.
- Defined Shader Toy, Render Toy, and the planned ANARI Device mode as separate product
  boundaries. Device-neutral native code uses `miskeyed::workbench`, while the shipped
  Slang/QRhi renderer retains its existing `slang_qrhi` namespace and behavior.
- Converted the ANARI research plan into an implementation breakdown that records the
  completed backend foundation and the remaining Hydra, tracing, frame-handoff, and
  Island-device slices.

## [0.2.1] — 2026-08-30

### Fixed

- Live recompilation now refreshes the parameter inspector after UI annotation changes.
- Constrained the package to the Python versions supported by Qt 6.8.
- The import-level `__version__` now comes from installed distribution metadata rather
  than reporting the stale prototype version.

## [0.2.0] — 2026-08-17

### Added

- Shader-driven UI metadata. Uniforms can be annotated with `[UIName]`,
  `[UIGroup]`, `[UIWidget]`, `[UIRange]`, `[UIStep]`, `[UITooltip]`, and
  `[UIUnits]` attributes; the parameter inspector reflects them to render grouped,
  labelled controls, with bounded floats getting a synced slider + spin box
  (`"slider"` / `"angle"` widgets). All built-in shaders and the Volumetric
  Clouds, Bloom, and CRT samples now ship annotated.
  The `UI*` attribute types are declared by a private "system prelude" the
  compiler injects ahead of every compile, so shaders annotate uniforms without
  pasting any boilerplate. Diagnostics stay mapped to the user's own line numbers
  via a `#line` reset, and the prelude is the seam for future plugin/sidecar
  metadata definitions.
- A pull-request CI workflow (`.github/workflows/ci.yml`) that builds the
  extension and runs the headless pytest suite across Python 3.11–3.13. It only
  rebuilds when C++/build inputs change, collapses overlapping push/PR runs into
  one, and exposes a single aggregate `CI` check to require in branch protection.
- Headless reflection tests (`tests/test_ui_metadata.py`).
- Developer tooling: `ruff` lint + format (config in `pyproject.toml`, enforced
  by a fast `lint` job in CI), a `.pre-commit-config.yaml` (ruff, clang-format,
  whitespace/EOF/YAML hooks), a `.clang-format` for the hand-written C++, a
  Dependabot config for GitHub Actions, and a `build.cmd` that builds in the
  project `.venv` with build isolation off (the reliable path — `uv`/isolated
  builds can't see the Qt-only `shiboken6-generator`).
- CI now caches the Qt SDK, the Slang SDK, and pip downloads, and compiles the
  C++ through `sccache`, so the shared native core is built once and reused across
  the Python matrix and later runs instead of recompiling every time.
- Release publishing is automatic: merging a version bump to `main` publishes to
  PyPI via Trusted Publishing once that version isn't already released, then
  records the matching `vX.Y.Z` tag and GitHub Release. Pushing a `v*` tag by hand
  still works as a manual re-release path.
- A `prepare-release` workflow skill and script that bumps the version across
  `pyproject.toml` and `CMakeLists.txt`, stamps the changelog, and verifies the
  build before handing off.

## [0.1.1] — 2026-08-16

### Added

- A `miskeyed-workbench` console entry point (alongside `workbench`) so
  `uvx miskeyed-workbench` runs without needing `--from`.

### Fixed

- Release now ships wheels for **all** supported Pythons. `install-qt-action`
  was hijacking the interpreter with its own Python 3.12, so every matrix job
  built a `cp312` wheel and only one survived on publish; disabled its
  `setup-python` so each job builds its correct `cp311` / `cp312` / `cp313` wheel.
- CI now asserts each freshly built wheel's ABI tag matches its matrix Python,
  so a wrong-interpreter build fails loudly instead of silently shipping one tag.
- README hero image uses an absolute raw-GitHub URL so it renders on PyPI.

## [0.1.0] — 2026-08-16

- Rebranded to **Workbench**, distributed as `miskeyed-workbench` / imported as
  `miskeyed.workbench` (PEP 420 namespace).
- Added a `workbench` console entry point.
- Modernized `pyproject.toml` for PyPI: PEP 639 SPDX `license` + `license-files`,
  split build-only `shiboken6-generator` out of runtime dependencies,
  `minimum-version = "build-system.requires"`, `sdist.exclude`, cached `build-dir`,
  and `[project.urls]`.
- The importable wheel no longer bundles the standalone executable
  (`SLANG_QRHI_BUILD_APP=OFF`).
- Added `.gitignore` and a tag-triggered GitHub Actions release workflow that
  builds Windows wheels + sdist and publishes to PyPI via Trusted Publishing.
- The build is fully environment-driven (Qt via `CMAKE_PREFIX_PATH`, Slang via
  `SLANG_ROOT`); no machine-specific paths remain and CI provisions both SDKs.
- Added a scalable SVG application icon (`assets/workbench.svg`, exported to
  `assets/workbench.ico` + PNG sizes) and applied it via `QApplication.setWindowIcon`.

Breaking reset; no compatibility layer with the previous pure-Python prototype.

- Replaced Python-owned rendering with a C++20 `QRhiWidget` renderer.
- Replaced `slangc`/`qsb` runtime subprocesses with the in-process Slang Compilation API.
- Added a Qt 6.8 `QShader` bridge for Slang-generated shader binaries and reflection metadata.
- Replaced comment parsing as the parameter source of truth with Slang reflection.
- Added native dynamic parameter model and inspector widgets.
- Added persistent shader-specific BLAKE2b Merkle DAG and dirty-state propagation.
- Added native standalone workbench.
- Added Shiboken6 bindings so the same Qt objects are directly usable from PySide6.
- Added reflected global-uniform binding propagation into QRhi resource bindings.
