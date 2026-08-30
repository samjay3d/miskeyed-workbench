Tool sessions and contributions
===============================

A **session** owns tool-specific runtime bindings and selected capabilities. A
**contribution** exposes a primary surface, actions, status, and summary to the
shell. The **workspace** owns documents, focus, and time. The **view selector** only
chooses a visible layout; it never defines session lifetime.

::

   ShaderWorkspace (documents/focus/time)
          | references
     +----+----------------+
     |                     |
   RenderToySession    ShaderToySession     both remain alive
     |                     |
   contribution view   contribution view    selector shows one layout

This is why Render Toy and Shader Toy coexist even though one primary view is
visible. A future Lookdev, Render, ANARI, or studio-specific tool can use the same
edge without taking ownership of shared services. ``QStackedWidget`` happens to
implement today's layout selection; it is not the architecture.

Adding a tool
-------------

1. Put reusable state in an explicit session.
2. Implement ``WorkbenchToolContribution`` at the composition edge.
3. Bind existing workspace documents rather than copying them.
4. Supply a surface and local actions; register it through
   ``WorkbenchToolFactory`` or a host composition root.
5. Keep graphics execution native and keep tool-specific state out of the editor.

The unshipped ``examples/python_tool_mode.py`` demonstrates exposure only; it is not
a Python render core.
