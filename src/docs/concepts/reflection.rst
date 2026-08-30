Reflection-driven UI
====================

Overview
--------

Reflection lets authored shader declarations describe controls without hard-coded
C++ widgets for every parameter.

Example
-------

::

   [UIGroup("Animation")]
   [UIName("Speed")]
   [UIWidget("slider")]
   [UIRange(0.05, 2.0)]
   [UIStep(0.01)]
   float speed = 1.0;

Mental model
------------

::

   Slang attributes -> reflection -> parameter description -> Inspector control

Metadata stays beside shader meaning. Workbench owns presentation and value storage;
Slang owns type/layout reflection. Host-managed values such as time are reflected for
binding but hidden from authored controls.

See also
--------

:doc:`../workbench_slang/ui_metadata` and :doc:`../architecture/reflection_pipeline`.
