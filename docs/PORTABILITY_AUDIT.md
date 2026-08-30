# Workbench portability audit

This is the boundary checklist for multi-platform work. An occurrence is not
automatically a defect: backend code is expected to know about graphics APIs and
packaging code is expected to know about shared-library conventions.

| Assumption | Classification | Owner / disposition |
|---|---|---|
| QRhi was selected with platform preprocessor branches in `SlangRhiWidget` | Accidental | Replaced by `RhiBackendPolicy`; `Auto` remains host-native and an explicit Vulkan selection works on Windows and Linux. |
| HLSL SM 5.0 is emitted for QRhi's D3D11 shader variant | Backend-specific | Remains in the Slang compiler target set. It does not select the live QRhi backend or the generated-code viewer. |
| SPIR-V and Metal code are emitted from the same Slang source | Backend-specific | Remains one compiler session and one authored source; no sample forks. |
| `slang.dll` and Windows DLL search registration | Platform-specific | Windows packaging edge only. CMake now also discovers `libslang.so` and `libslang.dylib`; installed native targets use a loader-relative runtime path. |
| `.pyd` module suffix and MSVC flags | Platform-specific | Correctly isolated in Shiboken/CMake conditionals. |
| Product stylesheet lived in Render Toy | Accidental | Moved to the shared `ui/WorkbenchTheme` boundary for reuse by future modes. |
| D3D11 resource retirement comments and tests | Backend-specific invariant | Kept: the conservative lifetime rule is safe for every QRhi backend even though D3D11 exposed the failure. |
| HLSL/GLSL/SPIR-V/Metal labels in the compiled-output UI | Generated-output-only | Kept independent of presentation backend selection. |
| `build.cmd`, PowerShell examples, ICO and Windows wheel jobs | Platform-specific | Retained as Windows entry points; portable CMake, SVG assets, and source builds are the cross-platform path. |

## Validation matrix

Release artifacts are produced for Windows x86_64 (D3D11 by default), Linux x86_64
(Vulkan), and macOS arm64/x86_64 (Metal), for every supported CPython 3.11–3.13 interpreter.
Each wheel is installed and imported in a new virtual environment before upload.

The policy contract is covered without creating a GPU device. The Windows CI build also
launches the native executable through D3D11 and Vulkan (using SwiftShader when the
hosted worker has no physical Vulkan device) and only passes after pipeline creation and
draw recording. Linux Vulkan is validated with the same smoke mode; Metal should use it
on a macOS GPU-capable worker. CPU/reflection tests remain separate so a missing CI GPU
is not mistaken for a compiler or workspace failure.
