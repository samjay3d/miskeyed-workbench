"""Capture deterministic teaching images from the real native Workbench UI."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path

from miskeyed.workbench import WorkbenchWindow
from PySide6.QtCore import QTimer
from PySide6.QtWidgets import (
    QApplication,
    QComboBox,
    QLabel,
    QPlainTextEdit,
    QPushButton,
    QSplitter,
    QTabWidget,
    QTreeWidget,
    QWidget,
)

from ci.verify_doc_images import EXPECTED


@dataclass(frozen=True)
class Scenario:
    filename: str
    target: str = "window"
    tool: str = "render-toy"
    focus: str = "scene"
    inspector: int = 0
    view: int = 0
    target_code: str = "HLSL"
    frame: int = 48
    minimum_size: tuple[int, int] = (280, 180)


SCENARIOS = {
    "overview": Scenario("workbench_overview.png", minimum_size=(1200, 700), view=2),
    "workspace-documents": Scenario(
        "workspace_documents.png", target="WorkspaceEditor", minimum_size=(700, 350)
    ),
    "documents-and-bindings": Scenario(
        "documents_and_bindings.png", target="WorkbenchRoot", minimum_size=(1000, 550)
    ),
    "inspector-parameters": Scenario(
        "inspector_parameters.png", target="InspectorPanel", minimum_size=(280, 450)
    ),
    "inspector-dependencies": Scenario(
        "inspector_dependencies.png", target="InspectorPanel", inspector=2, minimum_size=(280, 450)
    ),
    "inspector-entry-points": Scenario(
        "inspector_entry_points.png",
        target="InspectorPanel",
        tool="shader-toy",
        focus="shader",
        inspector=3,
        minimum_size=(280, 450),
    ),
    "timeline": Scenario("timeline_overview.png", target="Timeline", minimum_size=(700, 70)),
    "render-toy": Scenario(
        "render_toy.png", target="ToolSurface", focus="post", minimum_size=(700, 300)
    ),
    "shader-toy": Scenario(
        "shader_toy.png",
        target="ToolSurface",
        tool="shader-toy",
        focus="shader",
        minimum_size=(700, 300),
    ),
    "source-generated-compare": Scenario(
        "source_generated_compare.png",
        target="WorkspaceEditor",
        focus="post",
        view=2,
        minimum_size=(700, 350),
    ),
}

if {scenario.filename for scenario in SCENARIOS.values()} != set(EXPECTED):
    raise RuntimeError("capture scenarios and teaching-image manifest disagree")


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument("scenario", nargs="?", choices=SCENARIOS)
    result.add_argument("destination", nargs="?", type=Path)
    result.add_argument("--all", action="store_true", dest="capture_all")
    result.add_argument("--output", type=Path, default=Path("src/docs/images"))
    result.add_argument("--settle-ms", type=int, default=3000)
    return result


def required(window: QWidget, cls: type[QWidget], name: str):
    widget = window.findChild(cls, name)
    if widget is None:
        raise RuntimeError(f"required product widget {name!r} was not found")
    return widget


def click_view(window: QWidget, index: int) -> None:
    bar = required(window, QWidget, "DocumentViewBar")
    labels = ("Source", "Generated", "Compare")
    button = next((b for b in bar.findChildren(QPushButton) if b.text() == labels[index]), None)
    if button is None:
        raise RuntimeError(f"document view button {labels[index]!r} was not found")
    button.click()


def configure(window: WorkbenchWindow, scenario: Scenario) -> None:
    window.resize(1600, 950)
    window.setActiveTool(scenario.tool)
    if scenario.focus == "scene":
        window.sceneViewport().activated.emit()
    elif scenario.focus == "post":
        window.viewport().activated.emit()
    else:
        window.shaderToyViewport().activated.emit()

    tabs = required(window, QTabWidget, "ActiveDocumentInspector")
    tabs.setCurrentIndex(scenario.inspector)
    if scenario.inspector == 2:
        required(window, QTreeWidget, "DependencyTree").expandAll()
    if scenario.inspector == 3:
        required(window, QTreeWidget, "CompilationTree").expandAll()
    click_view(window, scenario.view)
    target = required(window, QComboBox, "WorkspaceGeneratedTarget")
    target_index = target.findText(scenario.target_code)
    if target_index >= 0:
        target.setCurrentIndex(target_index)

    window.timeTransport().setPlaying(False)
    timeline = required(window, QWidget, "Timeline")
    numeric = [
        widget
        for widget in timeline.findChildren(QWidget)
        if widget.metaObject().className() == "QDoubleSpinBox"
    ]
    if len(numeric) != 3:
        raise RuntimeError("timeline range/FPS controls are incomplete")
    numeric[0].setProperty("value", 0.0)
    numeric[1].setProperty("value", 240.0)
    numeric[2].setProperty("value", 24.0)
    required(timeline, QWidget, "TimelineScrubber").setProperty("value", scenario.frame)

    required(window, QSplitter, "WorkbenchRoot").setSizes([1120, 480])
    required(window, QSplitter, "DocumentWorkspace").setSizes([520, 380])
    for splitter in required(window, QWidget, "WorkspaceEditor").findChildren(QSplitter):
        splitter.setSizes([560, 560])


def capture_target(window: WorkbenchWindow, scenario: Scenario) -> QWidget:
    if scenario.target == "window":
        return window
    return required(window, QWidget, scenario.target)


def validate(window: WorkbenchWindow, scenario: Scenario) -> QWidget:
    expected = {
        "scene": window.sceneDocument(),
        "post": window.document(),
        "shader": window.shaderToyDocument(),
    }[scenario.focus]
    target = capture_target(window, scenario)
    checks = {
        "active tool": window.activeTool() == scenario.tool,
        "focused document": window.focusedDocument() == expected,
        "inspector tab": required(window, QTabWidget, "ActiveDocumentInspector").currentIndex()
        == scenario.inspector,
        "timeline visible": required(window, QWidget, "Timeline").isVisible(),
        "tool surface visible": required(window, QWidget, "ToolSurface").isVisible(),
        "source editor populated": bool(
            required(window, QPlainTextEdit, "WorkspaceSourceEditor").toPlainText()
        ),
        "binding summary populated": bool(
            required(window, QLabel, "BindingSummary").text().strip()
        ),
        "capture target visible": target.isVisible(),
        "capture target width": target.width() >= scenario.minimum_size[0],
        "capture target height": target.height() >= scenario.minimum_size[1],
    }
    if scenario.inspector == 2:
        checks["dependency rows"] = (
            required(window, QTreeWidget, "DependencyTree").topLevelItemCount() > 0
        )
    if scenario.inspector == 3:
        checks["entry point rows"] = (
            required(window, QTreeWidget, "CompilationTree").topLevelItemCount() > 0
        )
    failed = [name for name, passed in checks.items() if not passed]
    if failed:
        raise RuntimeError("scenario semantic validation failed: " + ", ".join(failed))
    return target


def capture_one(name: str, output: Path, settle_ms: int) -> int:
    app = QApplication.instance() or QApplication([])
    window = WorkbenchWindow()
    window.show()
    result = {"ok": False}

    def finish() -> None:
        try:
            scenario = SCENARIOS[name]
            configure(window, scenario)
            app.processEvents()
            target = validate(window, scenario)
            output.parent.mkdir(parents=True, exist_ok=True)
            result["ok"] = target.grab().save(str(output), "PNG") and output.stat().st_size > 0
        finally:
            window.close()
            app.quit()

    QTimer.singleShot(settle_ms, finish)
    app.exec()
    return 0 if result["ok"] else 1


def main() -> int:
    args = parser().parse_args()
    if args.capture_all == bool(args.scenario):
        parser().error("choose exactly one scenario or --all")
    jobs = SCENARIOS.items() if args.capture_all else [(args.scenario, SCENARIOS[args.scenario])]
    for name, scenario in jobs:
        output = args.output / scenario.filename if args.capture_all else args.destination
        if output is None:
            output = args.output / scenario.filename
        if capture_one(name, output, args.settle_ms):
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
