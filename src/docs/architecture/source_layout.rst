Native source layout
====================

Public headers under ``cpp/include/miskeyed/workbench/`` mirror implementations
under ``cpp/src/workbench/``:

``core/``
   Device-neutral identity, dependency, time, and camera value contracts. It does
   not contain widgets, compiler sessions, or mode policy.

``slang/``
   ``ShaderDocument``, ``ShaderWorkspace``, compiler/reflection models, and packaged
   module access. It owns Slang integration, not QRhi command submission.

``rendering/``
   QRhi backend policy, render passes, shader packaging, and viewport execution.
   It consumes compiled documents; it does not own workspace focus.

``editor/``
   Source/generated presentation, document tabs, syntax and language-service edges.
   Runtime bindings do not belong here.

``ui/``
   Reusable inspector, timeline, theme, view selector, and tool-contribution
   interfaces. Widgets adapt models rather than becoming authoritative state.

``modes/``
   Tool sessions and composition adapters. ``render_toy`` binds Scene/Post and
   currently contains the shell composition root; ``shader_toy`` owns its one
   binding. Shared services should move down, not be duplicated here.

``anari/``
   Optional device-neutral ANARI host discovery/lifetime work. It uses
   ``miskeyed::workbench`` rather than the historical ``slang_rhi`` namespace.

Packaged shader contracts are in ``shaders/workbench/``. Shiboken declarations are
in ``bindings/`` and the Python namespace package in ``python/miskeyed/workbench/``.
The native executable enters through ``app/main.cpp``.
