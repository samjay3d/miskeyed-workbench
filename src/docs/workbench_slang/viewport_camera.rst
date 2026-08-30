Viewport camera contract
========================

``import miskeyed.viewport_camera;`` exposes the host camera values used by Render
Toy scene programs. The host owns navigation and camera state; Slang owns how the
program interprets those values for rendering.

Scene samples import this contract rather than relying on an injected header. It
provides the uniform ``camera`` with ``camYaw``, ``camPitch``, ``camDistance``,
``camFov``, ``camPanX``, and ``camPanY``. Camera field names are centralized so
consumers and reflection agree.

Implementation
--------------

Relevant source: ``shaders/workbench/viewport_camera.slang`` and
``cpp/include/miskeyed/workbench/core/ViewportCamera.h``.
