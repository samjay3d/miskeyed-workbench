ANARI research boundary
=======================

ANARI Device is not a shipped Workbench UI mode in 0.3.0. The optional native host
foundation can discover configured ANARI libraries, enumerate devices/extensions,
report status, and exercise deterministic device lifetime. A standalone probe and
pinned CI test this narrow edge.

The proposed authoring path keeps USD as authored scene meaning, Hydra as change
propagation, and hdAnari as translation. The selected ANARI implementation owns its
render core and device resources; Workbench would own selection, lifecycle
coordination, diagnostics, authoring-session integration, and frame presentation.
It must not introduce a second USD traversal/database or merge ``ShaderDocument``
with ANARI device state.

Hydra integration, frame handoff, tracing, device-switch UX, and an ANARI tool
contribution remain research. The detailed record is retained in the repository's
history and should only enter this learning path when code and validation make it
current architecture.
