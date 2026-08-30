from pathlib import Path

import pytest

from ci.docs_metadata import development_url, project_metadata, versioned_url
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
    assert development_url(docs_url) == "https://samjay3d.github.io/miskeyed-workbench/dev/"
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

    prepare(site, publish, "release", "0.3.0", "https://samjay3d.github.io/miskeyed-workbench/")

    assert (publish / "0.2.1" / "index.html").read_text() == "old"
    assert (publish / "0.3.0" / "index.html").read_text() == "0.3.0"
    assert (
        "https://samjay3d.github.io/miskeyed-workbench/0.3.0/"
        in (publish / "index.html").read_text()
    )
    assert (publish / ".nojekyll").is_file()


def test_development_publication_is_replaceable_without_moving_stable(tmp_path):
    first = tmp_path / "first"
    second = tmp_path / "second"
    publish = tmp_path / "publish"
    write_site(first, "first dev")
    write_site(second, "second dev")
    write_site(publish / "0.3.0", "stable release")
    (publish / "index.html").write_text("stable redirect", encoding="utf-8")

    prepare(first, publish, "dev", "0.3.0", "https://samjay3d.github.io/miskeyed-workbench/")
    prepare(second, publish, "dev", "0.3.0", "https://samjay3d.github.io/miskeyed-workbench/")

    assert (publish / "dev" / "index.html").read_text() == "second dev"
    assert (publish / "0.3.0" / "index.html").read_text() == "stable release"
    assert (publish / "index.html").read_text() == "stable redirect"


def test_first_dev_publication_gives_the_empty_root_a_useful_redirect(tmp_path):
    site = tmp_path / "site"
    publish = tmp_path / "publish"
    write_site(site, "development")

    prepare(site, publish, "dev", "0.3.0", "https://samjay3d.github.io/miskeyed-workbench/")

    assert (
        "https://samjay3d.github.io/miskeyed-workbench/dev/" in (publish / "index.html").read_text()
    )


def test_published_version_is_immutable(tmp_path):
    site = tmp_path / "site"
    publish = tmp_path / "publish"
    write_site(site, "new")
    write_site(publish / "0.3.0", "already published")

    with pytest.raises(ValueError, match="immutable"):
        prepare(site, publish, "release", "0.3.0", "https://samjay3d.github.io/miskeyed-workbench/")


def test_release_workflow_gates_package_publication_on_live_docs():
    workflow = Path(".github/workflows/release.yml").read_text(encoding="utf-8")
    assert "needs: [detect, build-distributions, publish-documentation]" in workflow
    assert "Verify the public versioned documentation" in workflow
    assert "[Documentation for $v]($docs_url)" in workflow
    assert "permissions:\n            contents: write" in workflow


def test_ci_keeps_prs_read_only_and_publishes_dev_only_on_trusted_pushes():
    workflow = Path(".github/workflows/ci.yml").read_text(encoding="utf-8")
    assert "permissions:\n    contents: read" in workflow
    assert 'branches: [main, "release/**"]' in workflow
    assert "github.event_name == 'push'" in workflow
    assert "--site documentation-preview/site --publish-tree publish --channel dev" in workflow
    assert "[Public Dev Documentation]($url)" in workflow
