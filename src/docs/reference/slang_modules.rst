Slang module reference
======================

``miskeyed.ui``
---------------

:Kind: Library / language support
:Purpose: Semantic presentation annotations for reflected controls.
:Host backing: None; Workbench packages authored Slang declarations.
:Import: ``import miskeyed.ui;``
:Contract: ``UIName``, ``UIRange``, ``UIStep``, ``UIWidget``, ``UIGroup``,
   ``UITooltip``, and ``UIUnits`` attributes.
:Relevant consumer: Inspector reflection.
:Source trail: ``shaders/workbench/ui.slang``, ``WorkbenchModules.cpp``, and
   ``SlangCompiler.cpp``.

``miskeyed.time``
-----------------

:Kind: Host contract
:Purpose: Deterministic evaluation values shared by Workbench tools.
:Host backing: Workspace ``TimeContext`` adapted by each render surface.
:Import: ``import miskeyed.time;``
:Contract: Host-managed ``workbenchTime.time``, ``deltaTime``, ``frame``, and
   ``frameRate``.
:Relevant consumer: Render Toy and Shader Toy render surfaces.
:Source trail: ``shaders/workbench/time.slang``, ``TimeContext.h``,
   ``SlangRhiWidget.cpp``, and ``WorkbenchModules.cpp``.

``miskeyed.viewport_camera``
-----------------------------

:Kind: Host contract
:Purpose: Reflected camera state for viewport-aware scene programs.
:Host backing: Workbench viewport camera interaction.
:Import: ``import miskeyed.viewport_camera;``
:Contract: ``camera`` with yaw, pitch, distance, FOV, and pan fields.
:Relevant consumer: Render Toy scene/post viewports.
:Source trail: ``shaders/workbench/viewport_camera.slang``, ``ViewportCamera.h``,
   ``SlangRhiWidget.cpp``, and ``WorkbenchModules.cpp``.

``miskeyed.render_toy``
-----------------------

:Kind: Host contract
:Purpose: Scene/Post pass interface and scene-target sampling.
:Host backing: Render Toy's persistent session plus two-pass QRhi renderer.
:Import: ``import miskeyed.render_toy;``
:Contract: ``SceneSample``, host-bound ``sceneColor`` and ``sceneSampler``, and
   ``sampleScene``.
:Relevant consumer: ``RenderToySession`` Scene/Post bindings.
:Source trail: ``shaders/workbench/render_toy.slang``, ``RenderToySession.cpp``,
   ``SlangRhiWidget.cpp``, and ``WorkbenchModules.cpp``.

``miskeyed.shader_toy``
-----------------------

:Kind: Host contract
:Purpose: Fullscreen Shader Toy presentation inputs.
:Host backing: Shader Toy render surface pixel size.
:Import: ``import miskeyed.shader_toy;``
:Contract: Host-managed ``shaderToy.resolution``.
:Relevant consumer: ``ShaderToySession`` and its fullscreen render surface.
:Source trail: ``shaders/workbench/shader_toy.slang``, ``ShaderToySession.cpp``,
   ``SlangRhiWidget.cpp``, and ``WorkbenchModules.cpp``.

These descriptors document capabilities; Slang still owns module resolution.
