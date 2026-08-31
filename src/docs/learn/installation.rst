Install and run
===============

For a user
----------

The shortest path is a zero-install tool run with `uv`_::

   uvx miskeyed-workbench

The equivalent `pipx`_ one-shot command is::

   pipx run miskeyed-workbench

For a persistent command, use either::

   pipx install miskeyed-workbench
   miskeyed-workbench

or::

   uv tool install miskeyed-workbench
   miskeyed-workbench

.. _uv: https://docs.astral.sh/uv/getting-started/installation/
.. _pipx: https://pipx.pypa.io/stable/installation/

For a developer
---------------

A contributor clones the repository and builds the native extension against Qt 6.8
and Slang. That path is intentionally separate from running the tool::

   python -m pip install --no-build-isolation -e . -v
   python -m pytest tests/ -v

Set ``CMAKE_PREFIX_PATH`` and ``SLANG_ROOT`` to the environment-provided SDKs. See
:doc:`../extending/native_cpp` for ownership and build boundaries.
