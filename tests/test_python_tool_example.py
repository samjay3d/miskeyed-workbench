from __future__ import annotations

import ast
from pathlib import Path


def test_python_tool_example_uses_public_contribution_edge() -> None:
    source = Path("examples/python_tool_mode.py").read_text(encoding="utf-8")
    ast.parse(source)
    assert ".registerTool(" in source
    assert '.setActiveTool("studio-review")' in source
    assert ".focusedDocument()" in source
    assert ".timeContext()" in source
    assert "ShaderWorkspace(" not in source
    assert "SlangRhiWidget(" not in source
