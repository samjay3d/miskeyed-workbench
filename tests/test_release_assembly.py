import io
import json
import tarfile
import zipfile
from pathlib import Path

import pytest

from ci.assemble_release import CandidateError, assemble, create_candidate


def _candidate_input(
    root: Path, duplicate_payload: bool = False, versions: tuple[str, ...] = ("3.11",)
) -> None:
    for version in versions:
        for target in ("windows-x64", "linux-x64", "macos-arm64", "macos-x64"):
            artifact = f"wheel-{target}-py{version}"
            payload = f"wheels-{target}-py{version}.tgz"
            directory = root / artifact
            directory.mkdir(parents=True)
            if duplicate_payload and ("windows" in artifact or "linux" in artifact):
                payload = "wheels-3.11.tgz"
            wheel_name = f"example-1.0-{artifact.removeprefix('wheel-')}.whl"
            wheel = io.BytesIO()
            with zipfile.ZipFile(wheel, "w") as archive:
                archive.writestr("example/__init__.py", "")
            with tarfile.open(directory / payload, "w:gz") as archive:
                info = tarfile.TarInfo(wheel_name)
                info.size = len(wheel.getvalue())
                archive.addfile(info, io.BytesIO(wheel.getvalue()))
            validation = root / artifact.replace("wheel-", "validation-")
            validation.mkdir()
            (validation / "validation.json").write_text("{}")
    (root / "sdist").mkdir()
    (root / "sdist" / "example-1.0.tar.gz").write_bytes(b"sdist")


def _create(incoming: Path, candidate: Path, versions: list[str] | None = None) -> None:
    create_candidate(
        incoming,
        candidate,
        "1.0",
        "abc",
        "refs/heads/main",
        "owner/repo",
        "owner/repo/.github/workflows/release.yml@refs/heads/main",
        versions or ["3.11"],
    )


def test_candidate_round_trip(tmp_path: Path) -> None:
    incoming, candidate, dist = tmp_path / "in", tmp_path / "candidate", tmp_path / "dist"
    _candidate_input(incoming)
    _create(incoming, candidate)
    assemble(candidate, dist, "abc", "1.0")
    assert len(list(dist.glob("*.whl"))) == 4
    assert (dist / "example-1.0.tar.gz").is_file()
    manifest = json.loads((candidate / "candidate-manifest.json").read_text())
    assert manifest["source_sha"] == "abc"


def test_duplicate_transport_names_report_both_artifacts(tmp_path: Path) -> None:
    incoming = tmp_path / "in"
    _candidate_input(incoming, duplicate_payload=True)
    with pytest.raises(
        CandidateError,
        match=r"(?s)duplicate candidate payload: wheels-3.11.tgz.*wheel-linux.*wheel-windows",
    ):
        _create(incoming, tmp_path / "candidate")


def test_tampered_candidate_is_rejected(tmp_path: Path) -> None:
    incoming, candidate = tmp_path / "in", tmp_path / "candidate"
    _candidate_input(incoming)
    _create(incoming, candidate)
    next(candidate.glob("*.tgz")).write_bytes(b"broken")
    with pytest.raises(CandidateError, match="digest mismatch"):
        assemble(candidate, tmp_path / "dist", "abc", "1.0")


def test_workflows_preserve_candidate_boundary() -> None:
    build = Path(".github/workflows/build-distributions.yml").read_text()
    release = Path(".github/workflows/release.yml").read_text()
    assert "wheels-windows-x64-py${{ matrix.python-version }}.tgz" in build
    assert "name: release-candidate" in build
    assert "merge-multiple: true" not in release
    assert "name: release-candidate" in release
    assert "source_run_id:" in release and "run-id: ${{ inputs.source_run_id }}" in release
    validation_job = release.split("    validate-candidate-run:", 1)[1].split(
        "    resume-testpypi:", 1
    )[0]
    assert "uses: ./.github/workflows/build-distributions.yml" not in validation_job
    assert "gh api" not in validation_job
    assert "--require-release-matrix" in validation_job
    assert "inputs.source_run_id == '' && (" in release


def test_candidate_rejects_missing_platform(tmp_path: Path) -> None:
    incoming = tmp_path / "in"
    _candidate_input(incoming)
    missing = incoming / "wheel-macos-arm64-py3.11"
    for child in missing.iterdir():
        child.unlink()
    missing.rmdir()
    with pytest.raises(CandidateError, match="wheel artifact matrix mismatch.*macos-arm64"):
        _create(incoming, tmp_path / "candidate")


def test_release_policy_validates_producer_and_exact_matrix(tmp_path: Path) -> None:
    incoming, candidate = tmp_path / "in", tmp_path / "candidate"
    versions = ("3.11", "3.12", "3.13")
    _candidate_input(incoming, versions=versions)
    _create(incoming, candidate, list(versions))
    assemble(
        candidate,
        tmp_path / "dist",
        "abc",
        "1.0",
        "owner/repo",
        "owner/repo/.github/workflows/release.yml@",
        True,
    )
    with pytest.raises(CandidateError, match="candidate repository"):
        assemble(candidate, tmp_path / "wrong", "abc", "1.0", "other/repo")
