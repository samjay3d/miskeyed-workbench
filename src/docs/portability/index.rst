Portability evidence
====================

Workbench separates claims by evidence level:

* **build/package:** wheels are produced for Windows x86_64, Linux x86_64, and
  macOS arm64/x86_64;
* **fresh install/import:** release workflows validate every wheel;
* **UI construction:** headless Qt checks exercise native window composition;
* **runtime rendering:** Windows D3D11 and Vulkan are smoke-tested through draw
  recording; equivalent Linux Vulkan and macOS Metal rendering validation remains
  future work.

The canonical public documentation captures use Windows because it currently has
the strongest runtime evidence. Diagnostic captures from other platforms should
remain CI artifacts rather than being mixed into the teaching site.

See :doc:`../rendering/qrhi` for the backend boundary.
