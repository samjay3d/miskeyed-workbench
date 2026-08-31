Rendering pipeline
==================

::

   selected document entries -> QShader package -> RenderPass
       -> QRhi bindings/pipeline -> command recording -> presentation

Render Toy records Scene into an offscreen texture and Post samples it. ShaderToy
records one fullscreen consumer. Resources referenced by prior frames are retired for
at least ``maxFrameLatency + 1`` frames rather than freed synchronously.

Implementation
--------------

Relevant source: ``rendering/SlangRhiWidget.cpp``, ``RenderPass.cpp``, and
``Qt68ShaderBridge.cpp``.
