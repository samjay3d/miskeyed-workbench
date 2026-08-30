"""Workbench — native Qt 6.8 / QRhi Slang shader workbench (Shiboken6 bindings).

The C++ objects are the public API and are re-exported at the top level, e.g.
``from miskeyed.workbench import WorkbenchWindow``.
"""

from __future__ import annotations

import os
from pathlib import Path

__version__ = "0.1.0"


def _register_dll_directories() -> None:
    if not hasattr(os, "add_dll_directory"):  # Windows only
        return
    candidates = [Path(__file__).resolve().parent]  # slang DLLs bundled in the wheel
    env_dirs = os.environ.get("SLANG_QRHI_DLL_DIRS")
    if env_dirs:
        candidates += [Path(p) for p in env_dirs.split(os.pathsep)]
    slang_root = os.environ.get("SLANG_ROOT")
    if slang_root:  # editable/dev installs point here instead of bundling
        candidates.append(Path(slang_root) / "bin")
    seen = set()
    path_prepend = []
    for path in candidates:
        try:
            key = os.path.normcase(str(path))
            if path and key not in seen and path.is_dir():
                seen.add(key)
                os.add_dll_directory(str(path))
                path_prepend.append(str(path))
        except OSError:
            pass
    # Slang loads its downstream compilers (slang-glslang, spirv-opt) by name via
    # the standard search order, which add_dll_directory does not cover; PATH does.
    if path_prepend:
        os.environ["PATH"] = os.pathsep.join(path_prepend + [os.environ.get("PATH", "")])


def _preload_runtime() -> None:
    # Load the ABI-matched shiboken6 and Qt DLLs into the process *before* the native
    # extension so the loader resolves its imports to the already-loaded modules,
    # regardless of the DLL search path. `import PySide6` alone does not load Qt6*.dll;
    # importing the submodules does.
    try:
        import shiboken6  # noqa: F401
        from PySide6 import QtCore, QtGui, QtWidgets  # noqa: F401
    except ImportError:
        pass


_register_dll_directories()
_preload_runtime()


def app_icon():
    """Return the Workbench application QIcon (scalable SVG, valid at any size)."""
    from PySide6.QtGui import QIcon

    svg = Path(__file__).resolve().parent / "assets" / "workbench.svg"
    return QIcon(str(svg))


from . import _slang_qrhi as _ext  # noqa: E402

# The C++ classes live in the Workbench Slang RHI namespace inside the extension; surface
# them at the top level so callers use `miskeyed.workbench.WorkbenchWindow` directly.
_ns = _ext.miskeyed.workbench.slang_rhi
__all__ = [name for name in dir(_ns) if not name.startswith("_")]
for _name in __all__:
    globals()[_name] = getattr(_ns, _name)
del _name, _ns
