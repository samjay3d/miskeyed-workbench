"""Reflection tests for the UI metadata attributes on shader uniforms.

These are pure-CPU Slang reflection checks (no GPU), so they run on a headless
`offscreen` Qt platform. They require the built `miskeyed.workbench` extension and
the Slang runtime (SLANG_ROOT) to be importable.
"""

from __future__ import annotations

import os

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication  # noqa: E402

import miskeyed.workbench as workbench  # noqa: E402
from miskeyed.workbench import ShaderDocument, ShaderParameterModel  # noqa: E402


# The attribute types the Workbench understands, declared once so shaders can use them.
UI_META_PRELUDE = """
[__AttributeUsage(_AttributeTargets.Var)] struct UINameAttribute    { string name; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIRangeAttribute   { float min; float max; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIStepAttribute    { float step; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIWidgetAttribute  { string kind; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIGroupAttribute   { string group; }
[__AttributeUsage(_AttributeTargets.Var)] struct UITooltipAttribute { string text; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIUnitsAttribute   { string units; }
"""

SHADER = (
    UI_META_PRELUDE
    + """
[UIGroup("Camera")] [UIName("Field of View")] [UIWidget("slider")]
[UIRange(10.0, 120.0)] [UIStep(1.0)] [UITooltip("Vertical FOV")] [UIUnits("deg")]
uniform float camFov;

uniform float plain;   // no metadata -> falls back to defaults

struct VSOut { float4 position : SV_Position; float2 uv : TEXCOORD0; };

[shader("vertex")]
VSOut vsMain(uint vid : SV_VertexID)
{
    float2 p = float2((vid << 1) & 2, vid & 2);
    VSOut o;
    o.uv = p;
    o.position = float4(p * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}

[shader("fragment")]
float4 psMain(VSOut i) : SV_Target0 { return float4(i.uv, camFov + plain, 1.0); }
"""
)


@pytest.fixture(scope="module")
def app():
    instance = QApplication.instance() or QApplication([])
    yield instance


def _row_by_name(model: ShaderParameterModel, name: str) -> int:
    for row in range(model.rowCount()):
        idx = model.index(row, 0)
        if model.data(idx, ShaderParameterModel.Role.NameRole) == name:
            return row
    raise AssertionError(f"parameter {name!r} not reflected")


def _value(model: ShaderParameterModel, name: str, role) -> object:
    return model.data(model.index(_row_by_name(model, name), 0), role)


def test_ui_metadata_is_reflected(app):
    doc = ShaderDocument()
    doc.setSource(SHADER)
    doc.compile()

    model = doc.parameters()
    Role = ShaderParameterModel.Role

    assert _value(model, "camFov", Role.LabelRole) == "Field of View (deg)"
    assert _value(model, "camFov", Role.GroupRole) == "Camera"
    assert _value(model, "camFov", Role.WidgetRole) == "slider"
    assert _value(model, "camFov", Role.TooltipRole) == "Vertical FOV"
    assert float(_value(model, "camFov", Role.MinimumRole)) == pytest.approx(10.0)
    assert float(_value(model, "camFov", Role.MaximumRole)) == pytest.approx(120.0)
    assert float(_value(model, "camFov", Role.StepRole)) == pytest.approx(1.0)


def test_uniform_without_metadata_uses_defaults(app):
    doc = ShaderDocument()
    doc.setSource(SHADER)
    doc.compile()

    model = doc.parameters()
    Role = ShaderParameterModel.Role

    # A bare uniform still reflects; its label defaults to the parameter name and it
    # carries no widget hint (the inspector falls back to a plain numeric editor).
    assert _value(model, "plain", Role.LabelRole) in ("plain", "")
    assert not _value(model, "plain", Role.WidgetRole)


def test_app_icon_available(app):
    # Cheap smoke check that the package resources ship with the wheel.
    assert not workbench.app_icon().isNull()
