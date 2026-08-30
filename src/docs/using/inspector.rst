Use the Inspector
=================

The Inspector always describes the focused document. Its pages separate authored
controls, reflected resources, resolved dependencies, and compilation products.

Parameters
----------

.. image:: ../images/inspector_parameters.png
   :alt: Focused Parameters Inspector generated from Slang metadata
   :align: center

Notice that groups and bounded controls come from shader metadata. Host-managed time
fields are absent because the application, not the artist, drives them.

Host / Slang
------------

.. image:: ../images/host_features.png
   :alt: Focused Host and Slang Inspector showing available host contracts and focused-document import state
   :align: center

**Available** describes what the running host supplies. **Imported** is derived from
the focused document's compiler-resolved dependencies. Expand a feature for its
module, concise contract, provider, and session/tool context. Libraries such as
``miskeyed.ui`` are listed separately because they require no runtime provider.

Dependencies
------------

.. image:: ../images/inspector_dependencies.png
   :alt: Focused Dependencies Inspector showing the authored shader and resolved imports
   :align: center

Select an import to inspect its resolved source and identity.

Entry points
------------

.. image:: ../images/inspector_entry_points.png
   :alt: Focused Compilation Inspector showing reflected shader entry points
   :align: center

Compilation presents program capabilities and generated targets, not a required
``vsMain``/``psMain`` pair. Continue with :doc:`../concepts/entry_points`.
