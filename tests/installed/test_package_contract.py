"""Contracts that a wheel must satisfy without importing checkout implementation code."""

from __future__ import annotations

import os

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

import miskeyed.workbench as workbench  # noqa: E402
from PySide6.QtWidgets import QApplication  # noqa: E402


def test_installed_public_api_and_packaged_slang_modules() -> None:
    app = QApplication.instance() or QApplication([])
    assert app is not None
    assert workbench.WorkbenchWindow

    document = workbench.ShaderDocument()
    document.setSource(
        """
import miskeyed.time;
struct VSOut { float4 position : SV_Position; };
[shader("vertex")]
VSOut vsMain(uint vertexID : SV_VertexID)
{
    VSOut output;
    output.position = float4(float(vertexID), 0.0, 0.0, 1.0);
    return output;
}
[shader("fragment")]
float4 psMain() : SV_Target0 { return float4(workbenchTime.time, 0.0, 0.0, 1.0); }
"""
    )
    document.compile()

    assert document.compileSucceeded(), document.diagnostics()
    dependency_ids = [dependency.identity for dependency in document.importedDependencies()]
    assert "miskeyed.time" in dependency_ids
    assert "HLSL" in document.generatedTargets()
