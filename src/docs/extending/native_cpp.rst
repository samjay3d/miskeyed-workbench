Extend native C++
=================

Build requirements are C++20, CMake 3.24+, Qt/PySide/Shiboken 6.8.x, Slang, and
Python 3.11--3.13. SDK locations come from ``CMAKE_PREFIX_PATH`` and ``SLANG_ROOT``.

Choose the layer by ownership, not convenience: core values in ``core/``, compiler
integration in ``slang/``, QRhi consumption in ``rendering/``, reusable widgets in
``ui/``, and tool policy in ``modes/``. See :doc:`../architecture/source_layout`.
