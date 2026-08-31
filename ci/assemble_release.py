"""Build and consume the validated, immutable release-candidate envelope."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import tarfile
import zipfile
from pathlib import Path


class CandidateError(RuntimeError):
    pass


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _safe_members(archive: tarfile.TarFile, source: Path) -> list[tarfile.TarInfo]:
    members = archive.getmembers()
    for member in members:
        path = Path(member.name)
        if member.issym() or member.islnk() or path.is_absolute() or ".." in path.parts:
            raise CandidateError(f"unsafe archive member in {source}: {member.name}")
    return members


def create_candidate(
    source: Path, output: Path, version: str, source_sha: str, expected: int
) -> None:
    artifact_dirs = sorted(path for path in source.glob("wheel-*") if path.is_dir())
    if len(artifact_dirs) != expected:
        raise CandidateError(f"expected {expected} wheel artifacts, found {len(artifact_dirs)}")
    output.mkdir(parents=True, exist_ok=True)
    transports: list[dict[str, object]] = []
    wheel_names: dict[str, str] = {}
    payload_names: dict[str, str] = {}
    for artifact_dir in artifact_dirs:
        payloads = list(artifact_dir.glob("*.tgz"))
        if len(payloads) != 1:
            raise CandidateError(
                f"artifact {artifact_dir.name} contains {len(payloads)} transport payloads"
            )
        payload = payloads[0]
        if payload.name in payload_names:
            raise CandidateError(
                f"duplicate candidate payload: {payload.name}\nfrom:\n"
                f"  {payload_names[payload.name]}\n  {artifact_dir.name}"
            )
        payload_names[payload.name] = artifact_dir.name
        try:
            with tarfile.open(payload, "r:gz") as archive:
                members = _safe_members(archive, payload)
                wheels = [m for m in members if m.isfile() and m.name.endswith(".whl")]
                if len(wheels) != 1:
                    identity = f"{artifact_dir.name}/{payload.name}"
                    raise CandidateError(f"transport {identity} contains {len(wheels)} wheels")
                wheel = Path(wheels[0].name).name
        except (tarfile.TarError, EOFError) as error:
            raise CandidateError(
                f"invalid transport {artifact_dir.name}/{payload.name}: {error}"
            ) from error
        if wheel in wheel_names:
            raise CandidateError(
                f"duplicate wheel filename {wheel} in {wheel_names[wheel]} and {artifact_dir.name}"
            )
        wheel_names[wheel] = artifact_dir.name
        shutil.copy2(payload, output / payload.name)
        transports.append(
            {
                "artifact": artifact_dir.name,
                "file": payload.name,
                "sha256": _sha256(payload),
                "wheel": wheel,
            }
        )

    sdists = list((source / "sdist").glob("*.tar.gz"))
    if len(sdists) != 1:
        raise CandidateError(f"expected one sdist, found {len(sdists)}")
    shutil.copy2(sdists[0], output / sdists[0].name)
    validations = sorted(path.parent.name for path in source.glob("validation-*/validation.json"))
    if len(validations) != expected:
        raise CandidateError(f"expected {expected} validation results, found {len(validations)}")
    manifest = {
        "schema": 1,
        "version": version,
        "source_sha": source_sha,
        "wheels": transports,
        "sdist": {"file": sdists[0].name, "sha256": _sha256(sdists[0])},
        "validation": validations,
    }
    (output / "candidate-manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    )


def assemble(candidate: Path, output: Path, source_sha: str | None, version: str | None) -> None:
    manifest = json.loads((candidate / "candidate-manifest.json").read_text())
    if source_sha and manifest["source_sha"] != source_sha:
        raise CandidateError(
            f"candidate source SHA {manifest['source_sha']} does not match {source_sha}"
        )
    if version and manifest["version"] != version:
        raise CandidateError(f"candidate version {manifest['version']} does not match {version}")
    output.mkdir(parents=True, exist_ok=True)
    seen: set[str] = set()
    for entry in manifest["wheels"]:
        payload = candidate / entry["file"]
        if not payload.is_file() or _sha256(payload) != entry["sha256"]:
            raise CandidateError(f"candidate digest mismatch: {entry['artifact']}/{entry['file']}")
        try:
            with tarfile.open(payload, "r:gz") as archive:
                members = _safe_members(archive, payload)
                wheels = [m for m in members if m.isfile() and m.name.endswith(".whl")]
                if len(wheels) != 1 or Path(wheels[0].name).name != entry["wheel"]:
                    raise CandidateError(
                        f"manifest wheel mismatch in {entry['artifact']}/{entry['file']}"
                    )
                name = entry["wheel"]
                if name in seen:
                    raise CandidateError(f"duplicate wheel filename: {name}")
                seen.add(name)
                archive.extract(wheels[0], output, filter="data")
                extracted = output / wheels[0].name
                if extracted.parent != output:
                    extracted.replace(output / name)
                with zipfile.ZipFile(output / name) as wheel_zip:
                    bad = wheel_zip.testzip()
                    if bad:
                        raise CandidateError(f"wheel CRC failure in {name}: {bad}")
        except (tarfile.TarError, zipfile.BadZipFile, EOFError) as error:
            raise CandidateError(
                f"invalid transport {entry['artifact']}/{entry['file']}: {error}"
            ) from error
    sdist = candidate / manifest["sdist"]["file"]
    if not sdist.is_file() or _sha256(sdist) != manifest["sdist"]["sha256"]:
        raise CandidateError(f"candidate digest mismatch: {sdist.name}")
    shutil.copy2(sdist, output / sdist.name)


def main() -> None:
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)
    create = commands.add_parser("create")
    create.add_argument("--input", type=Path, required=True)
    create.add_argument("--output", type=Path, required=True)
    create.add_argument("--version", required=True)
    create.add_argument("--source-sha", required=True)
    create.add_argument("--expected-wheel-count", type=int, required=True)
    consume = commands.add_parser("assemble")
    consume.add_argument("--candidate", type=Path, required=True)
    consume.add_argument("--output", type=Path, required=True)
    consume.add_argument("--source-sha")
    consume.add_argument("--version")
    args = parser.parse_args()
    try:
        if args.command == "create":
            create_candidate(
                args.input, args.output, args.version, args.source_sha, args.expected_wheel_count
            )
        else:
            assemble(args.candidate, args.output, args.source_sha, args.version)
    except CandidateError as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
