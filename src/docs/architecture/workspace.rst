Workspace, documents, and bindings
==================================

``ShaderWorkspace`` is the authoring container. It owns open
``ShaderDocument`` objects, exactly one focused document, per-document
``DocumentSession`` view state, and shared time objects. It does **not** decide
which shader a tool renders.

``ShaderDocument`` owns source identity, live source, compiler diagnostics,
reflection, resources, entry points, generated targets, and its dependency graph.
A ``DocumentSession`` is deliberately cheaper: cursor/selection, scroll, Source /
Generated / Compare mode, and generated target selection.

Open, focused, and bound are different
--------------------------------------

::

   Workspace: A.slang, B.slang, C.slang
   Focus: C.slang

   Render Toy session: Scene -> A.slang, Post -> B.slang
   Shader Toy session: Shader -> C.slang

**Open** means the workspace preserves a document. **Focused** means its source and
semantic inspector are being presented. **Bound** means a runtime session consumes
it. Clicking a Render Toy viewport focuses that pass's bound document; merely
opening or focusing a file never silently rebinds a renderer.

A document can have more than one consumer. Binding is therefore a reference held
by a session, not transfer of ownership. Closing a document asks sessions to remove
their reference; switching the visible tool leaves every session alive.

Data flow
---------

::

   file/editor -> ShaderDocument -> workspace focus -> editor + inspector
                         |
                         +---- session binding ----> selected entry points -> viewport

Look in ``cpp/include/miskeyed/workbench/slang/ShaderWorkspace.h`` and
``ShaderDocument.h`` for ownership, ``editor/WorkspaceEditor.h`` for presentation,
and ``modes/*/*Session.h`` for bindings.
