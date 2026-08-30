from pathlib import Path

import pytest

from ci.docs_metadata import project_metadata, versioned_url
from ci.publish_docs import prepare
from ci.verify_doc_images import REQUIRED, verify


def write_site(root: Path, marker: str = "site") -> None:
    root.mkdir(parents=True)
    (root / "index.html").write_text(marker, encoding="utf-8")
    (root / "asset.txt").write_text(marker, encoding="utf-8")


def test_canonical_documentation_metadata_is_release_ready():
    version, docs_url = project_metadata()
    assert version == "0.3.0"
    assert docs_url == "https://samjay3d.github.io/miskeyed-workbench/"
    assert versioned_url(version, docs_url) == (
        "https://samjay3d.github.io/miskeyed-workbench/0.3.0/"
    )


def test_capture_manifest_reports_missing_and_accepts_complete_set(tmp_path):
    assert verify(tmp_path)
    for name in REQUIRED:
        (tmp_path / name).write_text("generated capture", encoding="utf-8")
    assert verify(tmp_path) == []


def test_publication_preserves_old_versions_and_updates_stable_root(tmp_path):
    site = tmp_path / "site"
    publish = tmp_path / "publish"
    write_site(site, "0.3.0")
    write_site(publish / "0.2.1", "old")

    prepare(site, publish, "0.3.0", "https://samjay3d.github.io/miskeyed-workbench/")

    assert (publish / "0.2.1" / "index.html").read_text() == "old"
    assert (publish / "0.3.0" / "index.html").read_text() == "0.3.0"
    assert (
        "https://samjay3d.github.io/miskeyed-workbench/0.3.0/"
        in (publish / "index.html").read_text()
    )
    assert (publish / ".nojekyll").is_file()


def test_published_version_is_immutable(tmp_path):
    site = tmp_path / "site"
    publish = tmp_path / "publish"
    write_site(site, "new")
    write_site(publish / "0.3.0", "already published")

    with pytest.raises(ValueError, match="immutable"):
        prepare(site, publish, "0.3.0", "https://samjay3d.github.io/miskeyed-workbench/")


def test_release_workflow_gates_package_publication_on_live_docs():
    workflow = Path(".github/workflows/release.yml").read_text(encoding="utf-8")
    assert "needs: [detect, build-distributions, publish-documentation]" in workflow
    assert "Verify the public versioned documentation" in workflow
    assert "[Documentation for $v]($docs_url)" in workflow
    assert "permissions:\n            contents: write" in workflow
