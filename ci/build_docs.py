"""Build the complete Workbench documentation site, including native UI captures."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

from sphinx.cmd.build import main as sphinx_main

from ci.docs_metadata import PROJECT_ROOT
from ci.verify_doc_images import verify


def build(output: Path, *, capture: bool = True, channel: str = "development") -> int:
    output = output.resolve()
    source = PROJECT_ROOT / "src" / "docs"
    images = source / "images"
    if output == source or source in output.parents:
        raise ValueError("Sphinx output must not be inside the authored documentation tree")
    if capture:
        subprocess.run(
            [
                sys.executable,
                "-m",
                "ci.capture_workbench",
                "--all",
                "--output",
                str(images),
            ],
            cwd=PROJECT_ROOT,
            check=True,
        )
    problems = verify(images)
    if problems:
        raise RuntimeError("; ".join(problems))
    if output.exists():
        shutil.rmtree(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    previous_channel = os.environ.get("WORKBENCH_DOCS_CHANNEL")
    os.environ["WORKBENCH_DOCS_CHANNEL"] = channel
    try:
        return sphinx_main(["-W", "--keep-going", "-b", "html", str(source), str(output)])
    finally:
        if previous_channel is None:
            os.environ.pop("WORKBENCH_DOCS_CHANNEL", None)
        else:
            os.environ["WORKBENCH_DOCS_CHANNEL"] = previous_channel


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("build/documentation/site"))
    parser.add_argument("--channel", choices=("development", "release"), default="development")
    parser.add_argument(
        "--skip-capture",
        action="store_true",
        help="Use only in tests when a complete generated capture set already exists",
    )
    args = parser.parse_args()
    return build(args.output, capture=not args.skip_capture, channel=args.channel)


if __name__ == "__main__":
    raise SystemExit(main())
