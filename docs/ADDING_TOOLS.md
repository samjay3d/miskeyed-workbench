# Adding a Workbench tool

A tool is a **session contribution**, not another application. The Workbench continues to
own open documents, focus, editor views, the inspector, diagnostics, and evaluation time.
A tool owns only its runtime bindings and the QWidget that represents its working surface.

## Native layout

The extension points are deliberately visible in the source tree:

```text
cpp/include/miskeyed/workbench/ui/WorkbenchToolFactory.h  interface + built-in catalog
cpp/src/workbench/ui/WorkbenchToolFactory.cpp             built-in composition root
cpp/include/miskeyed/workbench/modes/<tool>/              runtime session contract
cpp/src/workbench/modes/<tool>/                           runtime session implementation
examples/python_tool_mode.py                              unshipped Python example
```

For a shipped native tool:

1. Add a runtime session under `modes/<tool>`. It owns bindings, never Workspace documents.
2. Implement `WorkbenchToolContribution`. Build and data-bind the mode-specific views inside
   that implementation—not in `WorkbenchWindow`.
3. Add a small per-mode `create...Contribution()` function. Do not add arguments for the new
   mode to a central all-tools factory; the shell merely enumerates contribution values.
4. Keep editor, inspector, diagnostics, and timeline controls out of the tool surface.
5. Add a contract test proving bindings survive focus and active-tool changes.

There is intentionally no global mode enum. A studio-native composition root can construct
its own `WorkbenchToolContribution` and pass it to `registerToolContribution()`.

## Python example

[`examples/python_tool_mode.py`](../examples/python_tool_mode.py) is deliberately not a
shipped mode. It shows the smaller Python-facing edge:

```python
surface = StudioReviewTool(window)
window.registerTool("studio-review", "Studio Review", surface)
window.setToolStatus("studio-review", "Input: generated://review/material")
window.setActiveTool("studio-review")
```

`registerTool()` adds only the contributed QWidget to the selector. The example reads the
focused native document and shared native `TimeContext`; it does not create Python-owned
document, compiler, inspector, timeline, or render-core mirrors. Keep a Python reference to
the surface until it is unregistered.
