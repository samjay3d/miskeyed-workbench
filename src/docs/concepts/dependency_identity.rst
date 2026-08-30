Dependency identity
===================

Overview
--------

Workbench needs to decide whether a compiler or render product is still the same and
what work must happen after a change.

Mental model
------------

* A hash answers **“is this the same thing?”**
* Dirty state answers **“what work must happen again?”**

::

   scene.slang -> project.noise -> entry point -> parameter layout -> pipeline

Changing a UI label can rebuild the Inspector without changing uniform layout. A
slider normally uploads values only. Editing ``project.noise`` invalidates documents
that resolved that import.

The implementation is a persistent Merkle DAG, but that detail follows the concept.
See :doc:`../architecture/dependency_graph`.
