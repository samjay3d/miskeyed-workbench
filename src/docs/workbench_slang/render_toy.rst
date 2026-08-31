Render Toy contract
===================

``import miskeyed.render_toy;`` provides the pass contract for authored Render Toy
programs. ``SceneSample`` carries the scene color target. Post programs receive
the host-bound combined ``sceneColor`` sampler and the ``sampleScene(float2 uv)`` helper.

A Render Toy session chooses compatible vertex and fragment entry points for each
binding. The module does not own documents, focus, time, or QRhi resources.

Implementation
--------------

Relevant source: ``shaders/workbench/render_toy.slang`` and
``modes/render_toy/RenderToySession.*``.
