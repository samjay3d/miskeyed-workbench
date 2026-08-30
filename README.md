# Workbench

Workbench is a native Qt 6.8 technical-art application for editing Slang programs,
inspecting compiler/reflection results, and seeing changes through QRhi without an
export/relaunch loop.

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

## Install and run

```powershell
python -m pip install miskeyed-workbench
workbench
workbench eye.slang
```

The distribution is `miskeyed-workbench`; the Python import is
`miskeyed.workbench`. Wheels target CPython 3.11–3.13 on Windows x86_64, Linux
x86_64, and macOS arm64/x86_64.

## Platform and backend status

`Auto` selects D3D11 on Windows, Vulkan on Linux, and Metal on macOS; Windows may
also request Vulkan with `--rhi vulkan`. Packaging/import validation covers all
wheel platforms. Runtime draw smoke tests currently cover Windows D3D11 and Windows
Vulkan (SwiftShader). Linux/macOS packaging or UI construction should not be read as
full runtime-rendering validation. The active runtime backend is independent of the
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

- [Teaching documentation source](src/docs/index.rst) — deliberate learning path
- [Current architecture map](ARCHITECTURE.md)
- [Product vision](VISION.md)
- [0.3.0 release history](CHANGELOG.md)
- [Building, testing, screenshots, and docs](src/docs/contributing/index.rst)
- [Native source layout](src/docs/architecture/source_layout.rst)

Canonical screenshots and Sphinx HTML are generated deployment artifacts rather
than repository inputs. On a built Windows checkout, reproduce the complete site with:

```text
python -m pip install -r src/docs/requirements.txt
python ci/capture_workbench.py --all --output src/docs/images
python ci/verify_doc_images.py src/docs/images
sphinx-build -W --keep-going -b html src/docs build/documentation/docs/dev/local
```

CI packages review sites under `docs/dev/<ref>` without release permissions. The
rolling main site is staged at `docs/prod/main`; a `v0.3.0` release is staged at
`docs/prod/0.3.0`.

## License

MIT — see [LICENSE](LICENSE).
