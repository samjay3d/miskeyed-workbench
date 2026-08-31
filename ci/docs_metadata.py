"""Read the package version and canonical documentation URL from project metadata."""

from __future__ import annotations

import argparse
import tomllib
from pathlib import Path
from urllib.parse import urlparse

PROJECT_ROOT = Path(__file__).resolve().parents[1]
PYPROJECT = PROJECT_ROOT / "pyproject.toml"


def project_metadata(path: Path = PYPROJECT) -> tuple[str, str]:
    project = tomllib.loads(path.read_text(encoding="utf-8"))["project"]
    version = project["version"]
    docs_url = project.get("urls", {}).get("Documentation", "")
    parsed = urlparse(docs_url)
    if parsed.scheme != "https" or not parsed.netloc or not docs_url.endswith("/"):
        raise ValueError("project.urls.Documentation must be a canonical HTTPS URL ending in /")
    return version, docs_url


def development_url(docs_url: str) -> str:
    return f"{docs_url}dev/"


def versioned_url(version: str, docs_url: str) -> str:
    return f"{docs_url}{version}/"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--field",
        choices=("version", "documentation_url", "development_url", "versioned_url"),
        required=True,
    )
    parser.add_argument("--version", help="Version override for versioned_url")
    args = parser.parse_args()
    version, docs_url = project_metadata()
    values = {
        "version": version,
        "documentation_url": docs_url,
        "development_url": development_url(docs_url),
        "versioned_url": versioned_url(args.version or version, docs_url),
    }
    print(values[args.field])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
