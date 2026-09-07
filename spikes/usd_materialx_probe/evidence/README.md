# Recorded evidence

Generated on Linux x86_64 with the PyPI `MaterialX==1.39.5` wheel under CPython
3.14.4. `.\.venv\Scripts\python.exe probe.py generate --output <directory>` succeeded and produced the
preserved sources (the probe deterministically strips trailing blank lines and writes one final newline) and `SHA256SUMS` in this directory.

This proves upstream graph validation and Slang source generation only. No native
Workbench linkage, Slang compilation/reflection, QRhi packaging, draw, USD composition,
or installed-package resource loading was run. The host contract observed in the source
includes `vertexMain`, `fragmentMain`, position/normal/tangent/UV vertex inputs, vertex
and pixel constant buffers, environment/light values, and combined texture/sampler
values. The full source is retained so the next probe compiles exactly this product.
