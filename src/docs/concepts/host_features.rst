Host Features and Slang Modules
===============================

A host feature and a Slang module are related contracts, not the same object::

   Workbench host feature
       -> packaged Slang contract
       -> authored import
       -> reflected values/resources
       -> persistent session or render surface

For example, the workspace owns a ``TimeContext``. The render surface adapts its
current evaluation sample to ``miskeyed.time``. An authored shader imports that
contract and reads ``workbenchTime.time``::

   TimeContext
       -> miskeyed.time
       -> import miskeyed.time;
       -> workbenchTime.time

The **Host / Slang** Inspector page keeps two facts separate: **Available** means the
running Workbench host provides the capability; **Imported** means Slang actually
resolved that module for the focused document. Changing the visible tool does not
change availability because Render Toy and Shader Toy sessions remain alive together.

Library modules need no runtime provider. The shipped ``miskeyed.ui`` library only
declares semantic attributes consumed by reflection. A future pure library could use
the same category, but no unshipped module is implied.

The catalog makes this relationship discoverable; it does not resolve imports or
locate host services. Slang remains responsible for module semantics and resolution.
See :doc:`../workbench_slang/modules` for the shader-facing contracts.
