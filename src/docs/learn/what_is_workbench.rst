What is Workbench?
==================

Workbench is a desktop application for authoring Slang programs. A document is a
Slang source open in its workspace. A viewport shows a tool consuming a document;
the Inspector shows parameters and compiler knowledge for the focused document;
the timeline supplies an evaluation sample.

.. image:: ../images/workbench_overview.png
   :alt: Workbench orientation view with the major visible regions
   :align: center

The two shipped tools are deliberately small. **Render Toy** has Scene and Post
bindings. **ShaderToy** has one fullscreen binding. They share documents, focus,
time, and inspection instead of behaving as separate applications.

Next
----

Run the application in :doc:`installation`, then edit :doc:`first_shader`.
