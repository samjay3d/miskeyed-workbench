Portability and validation status
=================================

Workbench uses four evidence levels. **Wheel** means a platform-tagged distribution was
produced. **Installed** means that wheel installed and imported in a fresh virtual
environment. **Contracts** means the installed public API loaded the bundled Slang
runtime and compiled a representative shader using a packaged Workbench module.
**Runtime** means the canonical native harness created the named QRhi device and
pipeline and recorded a draw. A lower level never implies a higher one.

The 0.3.0 release gate covers CPython 3.11, 3.12, and 3.13. Native contracts and
runtime smoke execute once per platform on 3.11; the other Python lanes validate the
wheel and installed-package contract without claiming another render result.

.. list-table:: Release validation scope
   :header-rows: 1

   * - Platform
     - Wheels
     - Default QRhi backend
     - Runtime evidence required for release
   * - Windows x86_64
     - CPython 3.11--3.13
     - D3D11
     - Python 3.11: D3D11 and Vulkan/SwiftShader smoke
   * - Linux x86_64
     - CPython 3.11--3.13
     - Vulkan
     - Python 3.11: Vulkan smoke with the explicitly selected Mesa lavapipe ICD
   * - macOS arm64
     - CPython 3.11--3.13
     - Metal
     - Python 3.11: Metal smoke
   * - macOS x86_64
     - CPython 3.11--3.13
     - Metal
     - Python 3.11: Metal smoke

The workflow writes a per-lane Actions summary from actual step outcomes. Read
``success`` as evidence only for that column and lane. ``NOT VALIDATED`` means no
runtime smoke ran in that lane; it is not a failure and is not a runtime pass.
``UNSUPPORTED`` is reserved for a combination the product does not offer.

The latest release-candidate run is the source of truth for whether required evidence
actually passed. Support wording in release notes must be updated from those results,
not from the intended matrix or from wheel availability alone.
