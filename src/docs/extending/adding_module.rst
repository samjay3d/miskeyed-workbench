Add a Slang module
==================

1. Give the file a module declaration matching its search-path identity.
2. Export only the functions/types consumers need.
3. Put project modules on a Slang search path; reserve ``miskeyed.*`` for packaged
   Workbench contracts.
4. Import it normally and verify it appears in Dependencies.

::

   module studio.color;
   public float3 grade(float3 c) { return pow(c, 1.0 / 2.2); }

Workbench should not gain a second resolver for this extension.
