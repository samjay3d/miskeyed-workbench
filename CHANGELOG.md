# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
