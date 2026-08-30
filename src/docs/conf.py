import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
metadata = tomllib.loads((ROOT / "pyproject.toml").read_text(encoding="utf-8"))["project"]
version = metadata["version"]
documentation_url = metadata["urls"]["Documentation"]
project = "Miskeyed Workbench"
author = "Miskeyed Workbench contributors"
release = version
rst_epilog = f".. |workbench_release| replace:: {release}"
extensions = []
source_suffix = ".rst"
master_doc = "index"
exclude_patterns = ["_build"]
html_theme = "alabaster"
html_static_path = ["_static"]
html_title = f"Workbench {release} documentation"
html_baseurl = documentation_url
nitpicky = True
