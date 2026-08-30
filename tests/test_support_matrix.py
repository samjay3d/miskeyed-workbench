from __future__ import annotations

import json
import subprocess
import sys
import tomllib
from pathlib import Path


def _support() -> dict:
    return json.loads(Path("ci/support-matrix.json").read_text(encoding="utf-8"))


def test_support_metadata_agrees_with_package_workflow_and_docs() -> None:
    support = _support()
    project = tomllib.loads(Path("pyproject.toml").read_text(encoding="utf-8"))["project"]
    workflow = Path(".github/workflows/build-distributions.yml").read_text(encoding="utf-8")
    readme = Path("README.md").read_text(encoding="utf-8")
    docs = Path("src/docs/reference/portability.rst").read_text(encoding="utf-8")

    assert support["python"] == ["3.11", "3.12", "3.13"]
    assert project["requires-python"] == ">=3.11,<3.14"
    assert 'python-version: ["3.11", "3.12", "3.13"]' in workflow
    assert "3.11–3.13" in readme

    workflow_contracts = {
        "windows-x64": ("Windows x64", "--rhi d3d11", "--rhi vulkan"),
        "linux-x64": ("linux-x64", "--rhi vulkan"),
        "macos-arm64": ("macos-arm64", "--rhi metal"),
        "macos-x64": ("macos-x64", "--rhi metal"),
    }
    for platform in support["platforms"]:
        assert platform["label"] in readme
        assert platform["label"] in docs
        for marker in workflow_contracts[platform["id"]]:
            assert marker in workflow


def test_validation_summary_preserves_partial_failure(tmp_path: Path) -> None:
    lane = tmp_path / "lane.json"
    summary = tmp_path / "summary.md"
    subprocess.run(
        [
            sys.executable,
            "ci/validation_summary.py",
            "lane",
            "--platform",
            "Linux x64",
            "--python",
            "3.11",
            "--wheel",
            "success",
            "--installed",
            "success",
            "--installed-contracts",
            "success",
            "--native-contracts",
            "success",
            "--runtime",
            "Vulkan/lavapipe",
            "--runtime-result",
            "failure",
            "--output",
            str(lane),
        ],
        check=True,
    )
    subprocess.run(
        [
            sys.executable,
            "ci/validation_summary.py",
            "summary",
            "--input",
            str(tmp_path),
            "--output",
            str(summary),
        ],
        check=True,
    )
    report = summary.read_text(encoding="utf-8")
    assert "✅ Passed" in report
    assert "❌ Failed — Vulkan/lavapipe" in report
