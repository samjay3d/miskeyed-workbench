Time contract
=============

Import
------

::

   import miskeyed.time;

Provided fields
---------------

::

   workbenchTime.time
   workbenchTime.deltaTime
   workbenchTime.frame
   workbenchTime.frameRate

These are host-managed. Timeline or external evaluation changes upload new uniform
bytes without recompiling the program, and they do not appear as artist controls.

Example
-------

::

   float pulse = 0.5 + 0.5 * sin(workbenchTime.time * 2.0);

The core evaluation model remains richer than this GPU adapter; see
:doc:`../concepts/evaluation_time`.
