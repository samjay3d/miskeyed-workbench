"""Unshipped example: contribute a studio-only tool surface to Workbench.

This intentionally does not implement a renderer. It demonstrates that a Python studio
tool can join the native selector while reusing the open documents, inspector, editor,
timeline, and status bar owned by Workbench.
"""

from miskeyed.workbench import WorkbenchWindow
from PySide6.QtWidgets import QApplication, QLabel, QPushButton, QVBoxLayout, QWidget


class StudioReviewTool(QWidget):
    """Example working surface; production tools should live in a studio package."""

    def __init__(self, workbench: WorkbenchWindow) -> None:
        super().__init__()
        self._workbench = workbench
        self._document = QLabel("No document captured")
        self._time = QLabel()
        capture = QPushButton("Use focused document")
        capture.clicked.connect(self.capture_focused_document)

        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("Studio Review — Python contribution"))
        layout.addWidget(self._document)
        layout.addWidget(self._time)
        layout.addWidget(capture)
        layout.addStretch()

        workbench.timeContext().changed.connect(self.update_time)
        self.update_time()

    def capture_focused_document(self) -> None:
        document = self._workbench.focusedDocument()
        identity = document.fileUrl().toString() if document else "No focused document"
        self._document.setText(f"Preview input: {identity}")
        self._workbench.setToolStatus("studio-review", f"Input: {identity}")

    def update_time(self) -> None:
        self._time.setText(f"Shared time: {self._workbench.timeContext().timeSeconds():.3f} s")


def main() -> int:
    app = QApplication.instance() or QApplication([])
    window = WorkbenchWindow()

    # Keep the Python owner alive for as long as the native stack uses its QWidget.
    window.studio_review_tool = StudioReviewTool(window)
    if not window.registerTool("studio-review", "Studio Review", window.studio_review_tool):
        raise RuntimeError("The studio-review tool id is already registered")
    window.setActiveTool("studio-review")
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
