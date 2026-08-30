"""Verify the canonical documentation capture set before Sphinx runs."""

from __future__ import annotations

import argparse
from pathlib import Path

REQUIRED = (
    "workbench_overview.png",
    "render_toy.png",
    "shader_toy.png",
    "inspector_parameters.png",
    "inspector_dependencies.png",
    "inspector_compilation.png",
    "source_generated_compare.png",
    "timeline.png",
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", nargs="?", type=Path, default=Path("src/docs/images"))
    args = parser.parse_args()
    missing = [name for name in REQUIRED if not (args.directory / name).is_file()]
    empty = [
        name
        for name in REQUIRED
        if (args.directory / name).is_file() and not (args.directory / name).stat().st_size
    ]
    if missing or empty:
        if missing:
            print("missing documentation images: " + ", ".join(missing))
        if empty:
            print("empty documentation images: " + ", ".join(empty))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
