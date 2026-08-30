# Workbench

[![PyPI](https://img.shields.io/pypi/v/miskeyed-workbench.svg)](https://pypi.org/project/miskeyed-workbench/)
[![Python](https://img.shields.io/pypi/pyversions/miskeyed-workbench.svg)](https://pypi.org/project/miskeyed-workbench/)
[![CI](https://github.com/samjay3d/miskeyed-workbench/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/samjay3d/miskeyed-workbench/actions/workflows/ci.yml?query=branch%3Amain)
[![Documentation](https://img.shields.io/badge/docs-stable-blue.svg)](https://samjay3d.github.io/miskeyed-workbench/)
[![License](https://img.shields.io/pypi/l/miskeyed-workbench.svg)](LICENSE)

Workbench is a native Qt 6.8 technical-art application for editing Slang programs,
inspecting compiler/reflection results, and seeing changes through QRhi without an
export/relaunch loop.

## Run Workbench

For a zero-install launch on Python 3.11–3.13:

```text
uvx miskeyed-workbench
```

The equivalent command is `pipx run miskeyed-workbench`. For a persistent command,
use `pipx install miskeyed-workbench` or `uv tool install miskeyed-workbench`.

## Platform support

| Platform | Wheel | Python | Default backend | CI runtime validation |
|---|---|---|---|---|
| Windows x64 | ✅ Packaged | 3.11–3.13 | D3D11 | ✅ D3D11 + Vulkan/SwiftShader on Py3.11 |
| Linux x64 | ✅ Packaged | 3.11–3.13 | Vulkan | ✅ Vulkan/lavapipe on Py3.11 |
| macOS arm64 | ✅ Packaged | 3.11–3.13 | Metal | ✅ Metal on Py3.11 |
| macOS x86_64 | ✅ Packaged | 3.11–3.13 | Metal | ✅ Metal on Py3.11 |

**Supported** is the intended product contract. **Packaged** means a wheel is
published. **Tested** means installed-package and native contracts run. **Runtime
validated** means the named QRhi backend recorded a draw in the release matrix. See
the detailed [installation and platform support](src/docs/reference/portability.rst).

## What Workbench is

It currently ships two contributions inside one document-centric shell:

- **Render Toy** binds open documents to a Scene pass and a Post pass;
- **Shader Toy** binds one document to a minimal fullscreen shader consumer.

Both share the workspace, editor, reflection-driven Inspector, deterministic time
model, and native rendering services. ANARI host work remains opt-in research and is
not a shipped UI mode.

## Why Qt, Slang, and QRhi?

- **Slang** supplies language semantics, modules, in-process compilation, reflection,
  entry points, and generated backend code.
- **Qt** supplies the native application/widget lifecycle and Shiboken exposure.
- **QRhi** supplies one native rendering seam across D3D11, Vulkan, and Metal.

C++ owns lifecycle, synchronization, command submission, resources, and Python
exposure. There is no Python-owned renderer and no runtime `slangc` or `qsb`
subprocess.

The distribution is `miskeyed-workbench`; the developer/library import is
`miskeyed.workbench`. Wheels target CPython 3.11–3.13 on Windows x86_64, Linux
x86_64, and macOS arm64/x86_64.

## Platform and backend status

`Auto` selects D3D11 on Windows, Vulkan on Linux, and Metal on macOS; Windows may
also request Vulkan with `--rhi vulkan`. The release matrix separately records whether
each wheel built, installed in a fresh environment, passed installed-package contracts,
and rendered through a QRhi backend. The active runtime backend is independent of the
HLSL/GLSL/SPIR-V/Metal target selected in the Generated viewer.

## Build from source

Requirements are CMake 3.24+, C++20, Qt/PySide/Shiboken 6.8.x, Slang, and Python
3.11–3.13. Set `CMAKE_PREFIX_PATH` to the Qt development SDK and `SLANG_ROOT` to the
Slang SDK, then build without isolation:

```powershell
python -m pip install --no-build-isolation -e . -v
```

The matching `shiboken6-generator` comes from Qt's package index rather than PyPI;
the base dependencies alone are not enough for a source build.

## Learn and contribute

- **[Documentation](https://samjay3d.github.io/miskeyed-workbench/)** — current stable teaching site
- [Authored documentation source](src/docs/index.rst) — contributor-facing reStructuredText
- [Current architecture map](ARCHITECTURE.md)
- [Product vision](VISION.md)
- [0.3.0 release history](CHANGELOG.md)
- [Building, testing, screenshots, and docs](src/docs/contributing/index.rst)
- [Native source layout](src/docs/architecture/source_layout.rst)

Canonical screenshots and Sphinx HTML are generated deployment artifacts rather
than repository inputs. On a built Windows checkout, reproduce the complete site with:

```text
python -m pip install -r src/docs/requirements.txt
python -m ci.build_docs --output build/documentation/site
```

Pull requests upload the complete site as a read-only review artifact. Trusted
same-repository PRs also publish that artifact to `/dev/`; fork PRs never publish.
A release run publishes the immutable `/0.3.0/` snapshot to the generated `docs` branch
and updates the public root to redirect to that stable version.

## License

MIT — see [LICENSE](LICENSE).
