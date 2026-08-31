"""Prepare the generated-only GitHub Pages branch without deleting older releases."""

from __future__ import annotations

import argparse
import html
import shutil
from pathlib import Path

from ci.docs_metadata import project_metadata, versioned_url


def write_redirect(path: Path, target_url: str, title: str) -> None:
    escaped = html.escape(target_url, quote=True)
    path.write_text(
        "<!doctype html>\n"
        '<html lang="en"><head><meta charset="utf-8">\n'
        f'<meta http-equiv="refresh" content="0; url={escaped}">\n'
        f'<link rel="canonical" href="{escaped}">\n'
        f"<title>{html.escape(title)}</title></head>\n"
        f'<body><p><a href="{escaped}">{html.escape(title)}</a></p></body></html>\n',
        encoding="utf-8",
    )


def prepare(site: Path, publish_tree: Path, channel: str, version: str, docs_url: str) -> None:
    if not (site / "index.html").is_file():
        raise ValueError("generated Sphinx site has no index.html")
    publish_tree.mkdir(parents=True, exist_ok=True)
    if channel == "dev":
        target = publish_tree / "dev"
        if target.exists():
            shutil.rmtree(target)
        shutil.copytree(site, target)
        if not (publish_tree / "index.html").exists():
            write_redirect(
                publish_tree / "index.html",
                f"{docs_url}dev/",
                "Workbench development documentation",
            )
        (publish_tree / ".nojekyll").touch()
        return
    if channel != "release":
        raise ValueError(f"unsupported documentation channel: {channel}")
    target = publish_tree / version
    # The generated branch is authoritative once a version has been published. A retry
    # may run from a newer orchestration commit whose teaching sources have moved on;
    # preserving the existing tree makes immutability idempotent instead of turning the
    # already-published release documentation into an unrecoverable conflict.
    if not target.exists():
        shutil.copytree(site, target)
    redirect = versioned_url(version, docs_url)
    write_redirect(
        publish_tree / "index.html",
        redirect,
        f"Workbench {version} documentation",
    )
    (publish_tree / ".nojekyll").touch()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--site", type=Path, required=True)
    parser.add_argument("--publish-tree", type=Path, required=True)
    parser.add_argument("--channel", choices=("dev", "release"), required=True)
    parser.add_argument("--version")
    args = parser.parse_args()
    metadata_version, docs_url = project_metadata()
    prepare(
        args.site,
        args.publish_tree,
        args.channel,
        args.version or metadata_version,
        docs_url,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
