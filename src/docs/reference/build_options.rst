CMake build options
===================

``MISKEYED_WORKBENCH_BUILD_PYTHON``
   Build the Shiboken extension (on for wheels).
``MISKEYED_WORKBENCH_BUILD_APP``
   Build the standalone native executable (off for importable wheels).
``MISKEYED_WORKBENCH_WITH_ANARI``
   Enable the optional ANARI host foundation; this does not ship an ANARI UI mode.

SDK paths remain environment-driven rather than hard-coded into the repository.
