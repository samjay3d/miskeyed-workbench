from __future__ import annotations

import ast
import tomllib
from pathlib import Path

ROOT = Path(__file__).parents[1]
KIT = ROOT / "integrations" / "kit" / "exts"


def _manifest(name: str) -> dict:
    with (KIT / name / "config" / "extension.toml").open("rb") as stream:
        return tomllib.load(stream)


def test_slang_tools_declares_core_dependency_with_version_constraint():
    dependency = _manifest("miskeyed.workbench.slang_tools")["dependencies"][
        "miskeyed.workbench.core"
    ]
    assert dependency["version"] == "0.1.0"


def test_kit_imports_stay_out_of_native_and_base_python_sources():
    forbidden_roots = [ROOT / "cpp", ROOT / "bindings", ROOT / "python"]
    forbidden = ("omni", "carb")
    for source_root in forbidden_roots:
        for path in source_root.rglob("*"):
            if path.suffix not in {".py", ".h", ".cpp", ".xml"}:
                continue
            text = path.read_text(encoding="utf-8")
            assert not any(token in text for token in forbidden), path


def test_extension_modules_do_not_hide_missing_host_imports():
    for path in KIT.rglob("*.py"):
        tree = ast.parse(path.read_text(encoding="utf-8"))
        assert not any(isinstance(node, ast.Try) for node in ast.walk(tree)), path
