from pathlib import Path
from runpy import run_path

import pytest

from ci.docs_metadata import development_url, project_metadata, versioned_url
from ci.publish_docs import prepare
from ci.verify_doc_images import REQUIRED, verify

VERSION, DOCS_URL = project_metadata()


def write_site(root: Path, marker: str = "site") -> None:
    root.mkdir(parents=True)
    (root / "index.html").write_text(marker, encoding="utf-8")
    (root / "asset.txt").write_text(marker, encoding="utf-8")


def test_canonical_documentation_metadata_is_release_ready():
    assert DOCS_URL == "https://samjay3d.github.io/miskeyed-workbench/"
    assert development_url(DOCS_URL) == f"{DOCS_URL}dev/"
    assert versioned_url(VERSION, DOCS_URL) == f"{DOCS_URL}{VERSION}/"


def test_sphinx_version_identity_is_derived_from_project_metadata():
    config = run_path("src/docs/conf.py")
    assert config["release"] == VERSION
    assert config["version"] == ".".join(VERSION.split(".")[:2])
    assert config["html_title"] == f"Workbench {VERSION} documentation"


def test_capture_manifest_reports_missing_and_accepts_complete_set(tmp_path):
    assert verify(tmp_path)
    for name in REQUIRED:
        (tmp_path / name).write_text("generated capture", encoding="utf-8")
    assert verify(tmp_path) == []


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
    assert "Verify the public versioned documentation" in workflow
    assert "[Documentation for $v]($docs_url)" in workflow
    assert workflow.count("contents: write") == 2


def test_ci_keeps_builds_read_only_and_publishes_only_trusted_dev_inputs():
    workflow = Path(".github/workflows/ci.yml").read_text(encoding="utf-8")
    assert "permissions:\n    contents: read" in workflow
    assert 'branches: [main, "release/**"]' in workflow
    assert "github.event.pull_request.head.repo.full_name == github.repository" in workflow
    assert "github.event_name == 'push'" in workflow
    assert workflow.count("contents: write") == 1
    assert "--site documentation-preview/site --publish-tree publish --channel dev" in workflow
    assert "[Public Dev Documentation]($url)" in workflow
