"""Capture the native Workbench window for documentation in a running Qt event loop."""

from __future__ import annotations

import sys
from pathlib import Path

from miskeyed.workbench import WorkbenchWindow
from PySide6.QtCore import QTimer
from PySide6.QtWidgets import QApplication


def main() -> int:
    output = Path(sys.argv[1] if len(sys.argv) > 1 else "docs/images/workbench.png")
    output.parent.mkdir(parents=True, exist_ok=True)

    app = QApplication.instance() or QApplication([])
    window = WorkbenchWindow()
    window.show()

    saved = False
    inspector_synced = False

    def exercise_document_focus() -> None:
        """Exercise the same focus path used by viewport clicks before documenting it."""
        nonlocal inspector_synced
        inspector = window.parameterInspector()
        window.sceneViewport().activated.emit()
        scene_synced = (
            window.focusedDocument() == window.sceneDocument()
            and inspector.model() == window.sceneDocument().parameters()
        )
        window.viewport().activated.emit()
        post_synced = (
            window.focusedDocument() == window.document()
            and inspector.model() == window.document().parameters()
        )
        # Leave Scene focused so the capture makes the active viewport/document context visible.
        window.sceneViewport().activated.emit()
        inspector_synced = scene_synced and post_synced

    def capture() -> None:
        nonlocal saved
        saved = window.grab().save(str(output), "PNG")
        window.close()
        app.quit()

    # Let slangd startup, compilation, layout, and the first QRhi frames complete.
    QTimer.singleShot(2000, exercise_document_focus)
    QTimer.singleShot(3000, capture)
    app.exec()
    return 0 if inspector_synced and saved and output.stat().st_size > 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
