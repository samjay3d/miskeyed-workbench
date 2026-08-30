Entry points are capabilities
=============================

Overview
--------

A Slang program may expose multiple executable entry points.

Mental model
------------

::

   document provides capabilities
       -> tool session declares required stages
       -> binding selects exact entries
       -> renderer consumes them

Example
-------

::

   vertexMain   Vertex
   sceneMain    Fragment
   debugMain    Fragment
   inferMain    Compute

Render Toy can choose ``vertexMain + sceneMain`` while ShaderToy chooses
``vertexMain + debugMain``. Authored names identify user-facing capabilities; a
backend executable name may differ after code generation.

Implementation notes
--------------------

The document reflects all entries. Sessions retain selections without owning or
recompiling the document. ``vsMain`` and ``psMain`` are preferences, not architecture.

See also
--------

:doc:`../workbench_slang/entry_points`, :doc:`tool_sessions`, and
:doc:`../architecture/compilation`.
