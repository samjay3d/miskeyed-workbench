import io
import json
import tarfile
import zipfile
from pathlib import Path

import pytest

from ci.assemble_release import CandidateError, assemble, create_candidate


def _candidate_input(root: Path, duplicate_payload: bool = False) -> None:
    for artifact, payload in (
        ("wheel-windows-x64-py3.11", "wheels-windows-x64-py3.11.tgz"),
        ("wheel-linux-x64-py3.11", "wheels-linux-x64-py3.11.tgz"),
        ("wheel-macos-arm64-py3.11", "wheels-macos-arm64-py3.11.tgz"),
        ("wheel-macos-x64-py3.11", "wheels-macos-x64-py3.11.tgz"),
    ):
        directory = root / artifact
        directory.mkdir(parents=True)
        if duplicate_payload and ("windows" in artifact or "linux" in artifact):
            payload = "wheels-3.11.tgz"
        wheel_name = f"example-1.0-{artifact.removeprefix('wheel-')}-cp311.whl"
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


def test_candidate_round_trip(tmp_path: Path) -> None:
    incoming, candidate, dist = tmp_path / "in", tmp_path / "candidate", tmp_path / "dist"
    _candidate_input(incoming)
    create_candidate(incoming, candidate, "1.0", "abc", ["3.11"])
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
        create_candidate(incoming, tmp_path / "candidate", "1.0", "abc", ["3.11"])


def test_tampered_candidate_is_rejected(tmp_path: Path) -> None:
    incoming, candidate = tmp_path / "in", tmp_path / "candidate"
    _candidate_input(incoming)
    create_candidate(incoming, candidate, "1.0", "abc", ["3.11"])
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
    resume_section = release.split("    validate-candidate-run:", 1)[1]
    assert "uses: ./.github/workflows/build-distributions.yml" not in resume_section
    assert 'endswith("Seal validated release candidate")' in resume_section
    assert "(.wheels | length) == 12" in resume_section
    assert "inputs.source_run_id == '' && (" in release


def test_candidate_rejects_missing_platform(tmp_path: Path) -> None:
    incoming = tmp_path / "in"
    _candidate_input(incoming)
    missing = incoming / "wheel-macos-arm64-py3.11"
    for child in missing.iterdir():
        child.unlink()
    missing.rmdir()
    with pytest.raises(CandidateError, match="wheel artifact matrix mismatch.*macos-arm64"):
        create_candidate(incoming, tmp_path / "candidate", "1.0", "abc", ["3.11"])
