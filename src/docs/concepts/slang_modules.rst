Slang modules
=============

Overview
--------

A module is a named Slang unit that can be imported by another program.

Why it exists
-------------

Modules make shared shader behavior explicit and let Slang own language semantics,
visibility, composition, and resolution. Workbench does not invent a second resolver.

Example
-------

::

   module example.palette;
   public float3 accent() { return float3(0.2, 0.6, 1.0); }

and::

   import example.palette;

Workbench supplies packaged ``miskeyed.*`` modules and project search paths to the
Slang session, then records resolved imports for invalidation and inspection.

See also
--------

:doc:`../workbench_slang/modules` and :doc:`../architecture/module_resolution`.
