import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
metadata = tomllib.loads((ROOT / "pyproject.toml").read_text(encoding="utf-8"))["project"]
release = metadata["version"]
version = ".".join(release.split(".")[:2])
documentation_url = metadata["urls"]["Documentation"]
development_url = f"{documentation_url}dev/"
project = "Miskeyed Workbench"
author = "Miskeyed Workbench contributors"
rst_epilog = "\n".join(
    (
        f".. |workbench_release| replace:: {release}",
        f".. |documentation_url| replace:: {documentation_url}",
        f".. |development_url| replace:: {development_url}",
    )
)
extensions = []
source_suffix = ".rst"
master_doc = "index"
exclude_patterns = ["_build"]
html_theme = "alabaster"
html_static_path = ["_static"]
html_title = f"Workbench {release} documentation"
html_baseurl = documentation_url
nitpicky = True
