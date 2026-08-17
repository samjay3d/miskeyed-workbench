from __future__ import annotations
import sys
from PySide6.QtWidgets import QApplication
from . import WorkbenchWindow, app_icon


def main() -> int:
    app = QApplication.instance() or QApplication(sys.argv)
    app.setWindowIcon(app_icon())
    shader = sys.argv[1] if len(sys.argv) > 1 else ""
    window = WorkbenchWindow(shader) if shader else WorkbenchWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
