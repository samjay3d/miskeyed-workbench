import struct
import zlib
from pathlib import Path
from runpy import run_path

import pytest

from ci.docs_metadata import development_url, project_metadata, versioned_url
from ci.publish_docs import prepare
from ci.verify_doc_images import EXPECTED, verify

VERSION, DOCS_URL = project_metadata()


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    )


def write_png(path: Path, width: int, height: int) -> None:
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    pixels = b"".join(b"\0" + b"\0\0\0\xff" * width for _ in range(height))
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", header)
        + png_chunk(b"IDAT", zlib.compress(pixels))
        + png_chunk(b"IEND", b"")
    )


def write_site(root: Path, marker: str = "site") -> None:
    root.mkdir(parents=True)
    (root / "index.html").write_text(marker, encoding="utf-8")
    (root / "asset.txt").write_text(marker, encoding="utf-8")


def test_canonical_documentation_metadata_is_release_ready():
    assert DOCS_URL == "https://samjay3d.github.io/miskeyed-workbench/"
    assert development_url(DOCS_URL) == f"{DOCS_URL}dev/"
    assert versioned_url(VERSION, DOCS_URL) == f"{DOCS_URL}{VERSION}/"


def test_sphinx_version_identity_is_derived_from_project_metadata(monkeypatch):
    monkeypatch.delenv("WORKBENCH_DOCS_CHANNEL", raising=False)
    development = run_path("src/docs/conf.py")
    assert development["release"] == VERSION
    assert development["version"] == ".".join(VERSION.split(".")[:2])
    assert development["html_title"] == "Workbench development documentation"

    monkeypatch.setenv("WORKBENCH_DOCS_CHANNEL", "release")
    release = run_path("src/docs/conf.py")
    assert release["html_title"] == f"Workbench {VERSION}"


def test_capture_manifest_reports_missing_and_accepts_complete_set(tmp_path):
    assert verify(tmp_path)
    for name, minimum in EXPECTED.items():
        write_png(tmp_path / name, *minimum)
    assert verify(tmp_path) == []


def test_capture_manifest_rejects_corrupt_and_undersized_images(tmp_path):
    for name, minimum in EXPECTED.items():
        write_png(tmp_path / name, *minimum)
    corrupt = next(iter(EXPECTED))
    (tmp_path / corrupt).write_bytes(b"not a png")
    assert any("invalid documentation image" in problem for problem in verify(tmp_path))

    write_png(tmp_path / corrupt, 16, 16)
    assert any("undersized documentation image" in problem for problem in verify(tmp_path))


def test_publication_preserves_old_versions_and_updates_stable_root(tmp_path):
    site = tmp_path / "site"
    publish = tmp_path / "publish"
    write_site(site, VERSION)
    write_site(publish / "0.2.1", "old")

    prepare(site, publish, "release", VERSION, DOCS_URL)

    assert (publish / "0.2.1" / "index.html").read_text() == "old"
    assert (publish / VERSION / "index.html").read_text() == VERSION
    assert f"{DOCS_URL}{VERSION}/" in (publish / "index.html").read_text()
    assert (publish / ".nojekyll").is_file()


def test_development_publication_is_replaceable_without_moving_stable(tmp_path):
    first = tmp_path / "first"
    second = tmp_path / "second"
    publish = tmp_path / "publish"
    write_site(first, "first dev")
    write_site(second, "second dev")
    write_site(publish / VERSION, "stable release")
    (publish / "index.html").write_text("stable redirect", encoding="utf-8")

    prepare(first, publish, "dev", VERSION, DOCS_URL)
    prepare(second, publish, "dev", VERSION, DOCS_URL)

    assert (publish / "dev" / "index.html").read_text() == "second dev"
    assert (publish / VERSION / "index.html").read_text() == "stable release"
    assert (publish / "index.html").read_text() == "stable redirect"


def test_first_dev_publication_gives_the_empty_root_a_useful_redirect(tmp_path):
    site = tmp_path / "site"
    publish = tmp_path / "publish"
    write_site(site, "development")

    prepare(site, publish, "dev", VERSION, DOCS_URL)

    assert f"{DOCS_URL}dev/" in (publish / "index.html").read_text()


def test_published_version_is_immutable(tmp_path):
    site = tmp_path / "site"
    publish = tmp_path / "publish"
    write_site(site, "new")
    write_site(publish / VERSION, "already published")

    with pytest.raises(ValueError, match="immutable"):
        prepare(site, publish, "release", VERSION, DOCS_URL)


def test_release_workflow_gates_package_publication_on_live_docs():
    workflow = Path(".github/workflows/release.yml").read_text(encoding="utf-8")
    assert "needs: [detect, build-distributions, publish-documentation]" in workflow
    assert "python -m ci.build_docs --channel release" in workflow
    assert "Verify the public versioned documentation" in workflow
    assert "[PyPI $v]($pypi_url) · [Documentation $v]($docs_url)" in workflow
    assert workflow.count("contents: write") == 2


def test_ci_keeps_builds_read_only_and_publishes_only_trusted_dev_inputs():
    workflow = Path(".github/workflows/ci.yml").read_text(encoding="utf-8")
    assert "permissions:\n    contents: read" in workflow
    assert 'branches: [main, "release/**"]' in workflow
    assert "github.event.pull_request.head.repo.full_name == github.repository" in workflow
    assert "github.event_name == 'push'" in workflow
    assert workflow.count("contents: write") == 1
    assert "--site documentation-preview/site --publish-tree publish --channel dev" in workflow
    assert "[Development Documentation]($url)" in workflow
    assert "Pages propagation is asynchronous" in workflow
    assert 'grep -F "Workbench development documentation"' not in workflow
    assert 'grep -F "Workbench $version documentation"' not in workflow


def test_release_merge_gate_names_platform_package_contract_and_runtime_scope():
    distributions = Path(".github/workflows/build-distributions.yml").read_text(encoding="utf-8")
    release = Path(".github/workflows/release.yml").read_text(encoding="utf-8")
    stabilization = Path(".github/workflows/release-pr-artifacts.yml").read_text(encoding="utf-8")

    assert "github.event_name == 'pull_request'" in release
    assert "github.ref == 'refs/heads/main'" in release
    assert "release/0.3.0" not in release
    assert "confidence: Main Integration" in release
    assert 'python-versions: \'["3.11", "3.12", "3.13"]\'' in release
    assert "python-version: ${{ fromJSON(inputs.python-versions) }}" in distributions
    assert 'branches:\n            - "release/**"' in stabilization
    assert "confidence: Release Stabilization" in stabilization
    assert 'python-versions: \'["3.11", "3.13"]\'' in stabilization
    assert "pypi" not in stabilization.lower()
    assert "contents: write" not in stabilization
    assert "Wheel + Contracts + D3D11/Vulkan" in distributions
    assert "Wheel + Contracts + Vulkan" in distributions
    assert "Wheel + Contracts + Metal" in distributions
    assert "--rhi d3d11 --rhi-smoke-test" in distributions
    assert "--rhi vulkan --rhi-smoke-test" in distributions
    assert "--rhi metal --rhi-smoke-test" in distributions
    assert "mesa-vulkan-drivers" in distributions
    assert "VK_DRIVER_FILES" in distributions
    assert "tests/installed" in distributions
    assert "Workbench validation summary" in distributions
    assert "validation_summary.py summary" in distributions
    assert 'SLANG_ROOT: ""' in distributions
    assert "wheel-windows-x64-py${{ matrix.python-version }}" in distributions
    assert "validation-${{ matrix.target }}-py${{ matrix.python-version }}" in distributions
    assert "name: sdist" in distributions
    assert "pattern: wheel-*" in release
    assert "name: docs-release" in release


def test_confidence_ladder_labels_focused_ci_by_destination():
    workflow = Path(".github/workflows/ci.yml").read_text(encoding="utf-8")
    assert '"$BASE_REF" == main' in workflow
    assert '"$BASE_REF" == release/*' in workflow
    assert "level='Main Integration'" in workflow
    assert "level='Release Stabilization'" in workflow
    assert "level='Development CI'" in workflow
    assert "name: ${{ needs.changes.outputs.confidence }}" in workflow
