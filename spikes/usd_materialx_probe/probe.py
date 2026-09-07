#!/usr/bin/env python3
"""Optional, fail-loud USD/MaterialX fixture probe."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

HERE = Path(__file__).resolve().parent
FIXTURE = HERE / "fixture"


def blocker(command, detail):
    print(f"BLOCKED [{command}]: {detail}", file=sys.stderr)
    return 2


def check():
    root = (FIXTURE / "root.usda").read_text()
    geometry = (FIXTURE / "geometry.usda").read_text()
    look = (FIXTURE / "look.usda").read_text()
    material_path = FIXTURE / "materials/look.mtlx"
    material = ET.parse(material_path).getroot()
    texture = FIXTURE / "textures/base_color.ppm"
    assert material.tag == "materialx" and material.attrib["version"] == "1.39"
    assert material.find("./standard_surface") is not None
    assert material.find("./surfacematerial") is not None
    image_file = material.find("./image/input[@name='file']")
    assert image_file is not None
    assert (material_path.parent / image_file.attrib["value"]).resolve() == texture.resolve()
    assert texture.read_text(encoding="ascii").split()[:4] == ["P3", "2", "2", "255"]
    assert "@materials/look.mtlx@" in root and "@look.usda@" in root
    assert "normal3f[] normals" in geometry and "primvars:st" in geometry
    assert "</MaterialX/Materials/WorkbenchMaterial>" in look
    evidence = HERE / "evidence/generated"
    expected = {}
    for line in (evidence / "SHA256SUMS").read_text().splitlines():
        digest, path = line.split(maxsplit=1)
        expected[Path(path).name] = digest
    for name, digest in expected.items():
        source = (evidence / name).read_bytes()
        assert hashlib.sha256(source).hexdigest() == digest
    assert (
        b"vertexMain(VertexInputs vi)" in (evidence / "WorkbenchMaterial.vertex.slang").read_bytes()
    )
    assert (
        b"fragmentMain(VertexData _vd)" in (evidence / "WorkbenchMaterial.pixel.slang").read_bytes()
    )
    print(json.dumps({"fixture": str(FIXTURE), "static_contract": "pass"}, indent=2))
    return 0


def usd_roundtrip(reopen=None):
    try:
        from pxr import Sdf, Usd, UsdShade
    except ImportError as exc:
        return blocker("usd-roundtrip", f"OpenUSD Python bindings unavailable: {exc}")
    if reopen:
        stage = Usd.Stage.Open(str(reopen / "root.usda"))
        if not stage:
            return blocker("usd-roundtrip", "fresh process could not open composed stage")
        mesh = stage.GetPrimAtPath("/Fixture/Mesh")
        material, _ = UsdShade.MaterialBindingAPI(mesh).ComputeBoundMaterial()
        value = (
            stage.GetPrimAtPath("/MaterialX/Materials/WorkbenchMaterial")
            .GetAttribute("inputs:specular_roughness")
            .Get()
        )
        if not material or value != 0.42:
            return blocker(
                "usd-roundtrip", f"reopen mismatch: material={material}, roughness={value}"
            )
        print(json.dumps({"usd_fresh_process_reopen": "pass", "roughness": value}, indent=2))
        return 0
    import shutil

    with tempfile.TemporaryDirectory(prefix="miskeyed-usd-mtlx-") as temp:
        destination = Path(temp) / "fixture"
        shutil.copytree(FIXTURE, destination)
        stage = Usd.Stage.Open(str(destination / "root.usda"))
        if not stage:
            return blocker("usd-roundtrip", "stage open failed; verify usdMtlx plugin/resources")
        layer = Sdf.Layer.FindOrOpen(str(destination / "look.usda"))
        if not layer:
            return blocker("usd-roundtrip", "explicit look edit layer was not found")
        stage.SetEditTarget(layer)
        stage.OverridePrim("/MaterialX/Materials/WorkbenchMaterial").CreateAttribute(
            "inputs:specular_roughness", Sdf.ValueTypeNames.Float
        ).Set(0.42)
        layer.Save()
        return subprocess.run(
            [sys.executable, str(Path(__file__).resolve()), "_reopen", str(destination)],
            check=False,
        ).returncode


def generate(output):
    try:
        import MaterialX as mx
        import MaterialX.PyMaterialXGenShader as mx_gen
        import MaterialX.PyMaterialXGenSlang as mx_slang
    except ImportError as exc:
        return blocker("generate", f"MaterialX Slang generator bindings unavailable: {exc}")
    document = mx.createDocument()
    search_path = mx.getDefaultDataSearchPath()
    mx.loadLibraries(["libraries"], search_path, document)
    mx.readFromXmlFile(document, str(FIXTURE / "materials/look.mtlx"), search_path)
    valid, message = document.validate()
    if not valid:
        return blocker("generate", f"MaterialX validation failed: {message}")
    element = document.getNode("WorkbenchMaterial")
    if element is None:
        return blocker("generate", "WorkbenchMaterial was not resolved")
    generator = mx_slang.SlangShaderGenerator.create()
    context = mx_gen.GenContext(generator)
    context.registerSourceCodeSearchPath(search_path)
    shader = generator.generate("WorkbenchMaterial", element, context)
    output.mkdir(parents=True, exist_ok=True)
    stages = {}
    for name in ("vertex", "pixel"):
        source = shader.getStage(name).getSourceCode()
        path = output / f"WorkbenchMaterial.{name}.slang"
        path.write_text(source, encoding="utf-8")
        stages[name] = {"bytes": len(source.encode()), "path": str(path)}
    print(json.dumps({"materialx_version": mx.getVersionString(), "stages": stages}, indent=2))
    return 0


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("check")
    sub.add_parser("usd-roundtrip")
    gen = sub.add_parser("generate")
    gen.add_argument("--output", type=Path, default=HERE / "generated")
    reopen = sub.add_parser("_reopen", help=argparse.SUPPRESS)
    reopen.add_argument("fixture", type=Path)
    args = parser.parse_args()
    if args.command == "check":
        return check()
    if args.command == "usd-roundtrip":
        return usd_roundtrip()
    if args.command == "_reopen":
        return usd_roundtrip(args.fixture)
    return generate(args.output)


if __name__ == "__main__":
    raise SystemExit(main())
