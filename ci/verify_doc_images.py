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


def verify(directory: Path) -> list[str]:
    missing = [name for name in REQUIRED if not (directory / name).is_file()]
    empty = [
        name
        for name in REQUIRED
        if (directory / name).is_file() and not (directory / name).stat().st_size
    ]
    problems = []
    if missing:
        problems.append("missing documentation images: " + ", ".join(missing))
    if empty:
        problems.append("empty documentation images: " + ", ".join(empty))
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", nargs="?", type=Path, default=Path("src/docs/images"))
    args = parser.parse_args()
    problems = verify(args.directory)
    for problem in problems:
        print(problem)
    return 1 if problems else 0


if __name__ == "__main__":
    raise SystemExit(main())
