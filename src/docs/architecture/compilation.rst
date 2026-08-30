Compilation pipeline
====================

::

   editor buffer
      -> Slang session + module resolution
      -> program composition and link
      -> reflection: entries, parameters, resources
      -> generated backend blobs
      -> QShader bridge
      -> consumer pipeline invalidation

Compilation is in-process; Workbench launches neither ``slangc`` nor ``qsb``. A
failed compile retains diagnostics on the document instead of transferring compiler
ownership to the UI.

Implementation
--------------

Relevant source: ``SlangCompiler.cpp``, ``ShaderDocument.cpp``, and
``Qt68ShaderBridge.cpp``.
