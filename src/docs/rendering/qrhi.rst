QRhi, backends, and portability
===============================

Qt's Rendering Hardware Interface keeps the viewport contract stable across native
APIs::

   Workbench -> QRhi -> D3D11 (Windows)
                      Vulkan (Windows/Linux)
                      Metal (macOS)

``RhiBackendPolicy`` selects one backend for the viewport set. ``SlangRhiWidget``
and ``RenderPass`` own native execution state: QRhi buffers, bindings, pipelines,
recording, and deferred resource retirement. A session supplies a document and
entry-point choice; it does not submit commands.

Runtime and inspection are orthogonal
-------------------------------------

The active RHI says where frames execute. The Generated viewer target says which
compiler output a reader wants to inspect. Running Vulkan while viewing HLSL is a
valid configuration and does not reconfigure the device.

Support evidence
----------------

0.3.0 packages CPython 3.11--3.13 wheels for Windows x86_64, Linux x86_64, and
macOS arm64/x86_64. Fresh-wheel import validates packaging on all three families.
Windows CI additionally smoke-tests D3D11 and Vulkan through device creation,
pipeline creation, and draw recording. Linux/macOS package and UI-construction
evidence must not be read as equivalent runtime-rendering validation. Metal is a
policy/code path, not yet the canonical public capture platform.

QRhi resources potentially referenced by prior frames are retired for at least
``maxFrameLatency + 1`` frames; synchronously freeing them from rendering code can
produce backend use-after-free.
