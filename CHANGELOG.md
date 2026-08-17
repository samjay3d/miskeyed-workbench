# Changelog

## Unreleased — packaging

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
