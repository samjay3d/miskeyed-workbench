import os
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
metadata = tomllib.loads((ROOT / "pyproject.toml").read_text(encoding="utf-8"))["project"]
release = metadata["version"]
version = ".".join(release.split(".")[:2])
documentation_url = metadata["urls"]["Documentation"]
development_url = f"{documentation_url}dev/"
channel = os.environ.get("WORKBENCH_DOCS_CHANNEL", "development")
if channel not in {"development", "release"}:
    raise ValueError(f"unknown documentation channel: {channel}")
docs_identity = (
    "Workbench development documentation" if channel == "development" else f"Workbench {release}"
)
project = "Miskeyed Workbench"
author = "Miskeyed Workbench contributors"
rst_epilog = "\n".join(
    (
        f".. |workbench_release| replace:: {release}",
        f".. |documentation_url| replace:: {documentation_url}",
        f".. |development_url| replace:: {development_url}",
        f".. |docs_identity| replace:: {docs_identity}",
    )
)
extensions = []
source_suffix = ".rst"
master_doc = "index"
exclude_patterns = ["_build"]
html_theme = "furo"
html_static_path = ["_static"]
html_title = docs_identity
html_baseurl = documentation_url
nitpicky = True
