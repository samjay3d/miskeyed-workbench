# USD/MaterialX authoring fixture

This optional research fixture is not a shipped Workbench feature. See
[`src/docs/research/usd_materialx.rst`](../../src/docs/research/usd_materialx.rst) for
the decisions, pinned experiment envelope, evidence status, and commands.

`probe.py check` has no third-party dependencies. The other commands fail with an
actionable blocker unless a compatible native OpenUSD/MaterialX build and its Python
bindings, plugins, and data libraries are visible.
