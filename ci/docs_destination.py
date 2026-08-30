"""Choose the review or production documentation output for a Git ref."""

from __future__ import annotations

import argparse
import re
from pathlib import Path, PurePosixPath

_SAFE_COMPONENT = re.compile(r"[^A-Za-z0-9._-]+")


def _components(ref: str) -> list[str]:
    value = ref.strip().replace("\\", "/")
    for prefix in ("refs/heads/", "refs/tags/"):
        if value.startswith(prefix):
            value = value[len(prefix) :]
            break
    parts = [_SAFE_COMPONENT.sub("-", part).strip("-.") for part in value.split("/")]
    return [part for part in parts if part]


def destination(ref: str, version: str) -> PurePosixPath:
    """Return a Pages-relative destination without touching the filesystem."""
    raw = ref.strip().replace("\\", "/")
    parts = _components(raw)
    if not parts:
        raise ValueError("documentation ref is empty")
    if raw in {"main", "refs/heads/main"}:
        return PurePosixPath("docs/prod/main")
    if raw in {f"v{version}", f"refs/tags/v{version}"}:
        return PurePosixPath("docs/prod") / version
    if parts[0] == "release":
        if len(parts) < 2:
            raise ValueError("release refs must include a version")
        return PurePosixPath("docs/dev/release").joinpath(*parts[1:])
    return PurePosixPath("docs/dev/branch").joinpath(*parts)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ref", required=True, help="Head branch or full Git ref")
    parser.add_argument("--version", required=True, help="Release version without a v prefix")
    parser.add_argument("--root", type=Path, default=Path("build/documentation"))
    args = parser.parse_args()
    print(args.root.joinpath(*destination(args.ref, args.version).parts))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
