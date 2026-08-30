Slang modules and compilation
=============================

Workbench embeds Slang's C++ API because compilation, reflection, and dependency
results must participate in one live native transaction. It never launches
``slangc`` or ``qsb`` at runtime.

A typical authored program imports explicit contracts::

   import miskeyed.ui;
   import miskeyed.time;
   import miskeyed.viewport_camera;
   import miskeyed.render_toy;

These sources live in ``shaders/workbench/`` and are packaged with the application.
A small ``ISlangFileSystem`` edge exposes packaged modules. Project and user modules
remain ordinary Slang search paths. Slang, rather than a Workbench header catalog,
owns import semantics and resolution.

::

   editor source -> loadModuleFromSourceString
      + packaged filesystem + project search paths
      -> compose entry points -> link -> reflection + backend code

Slang owns language meaning, imports, linking, reflection, and code generation.
Workbench owns unsaved editor buffers, watches resolved dependencies, records their
identity, presents diagnostics, and binds compiler products to consumers. The old
concatenated prelude/header-browser design is not part of 0.3.0.

See ``cpp/src/workbench/slang/SlangCompiler.cpp`` and ``WorkbenchModules.cpp``.
