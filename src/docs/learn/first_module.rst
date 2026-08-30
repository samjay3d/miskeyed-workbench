Your first Slang module
=======================

Create ``example/noise.slang``::

   module example.noise;

   public float noise(float3 p)
   {
       return frac(sin(dot(p, float3(12.9, 78.2, 37.7))) * 43758.5);
   }

Import it from a shader on the project search path::

   import example.noise;

Slang owns the module name, import, and language semantics. Workbench records the
resolved dependency so an edit to ``example.noise`` invalidates its consumers.
Continue with :doc:`../workbench_slang/modules`.
