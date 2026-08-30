"""Capture deterministic documentation scenarios from the real native Workbench UI."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path

from miskeyed.workbench import WorkbenchWindow
from PySide6.QtCore import QTimer
from PySide6.QtWidgets import (
    QApplication,
    QComboBox,
    QPlainTextEdit,
    QPushButton,
    QSplitter,
    QTabWidget,
    QWidget,
)


@dataclass(frozen=True)
class Scenario:
    filename: str
    tool: str = "render-toy"
    focus: str = "scene"
    inspector: int = 0
    view: int = 0
    target: str = "HLSL"
    frame: int = 48


SCENARIOS = {
    "overview": Scenario("workbench_overview.png", view=2),
    "render-toy": Scenario("render_toy.png", focus="post"),
    "shader-toy": Scenario("shader_toy.png", tool="shader-toy", focus="shader"),
    "inspector-parameters": Scenario("inspector_parameters.png", inspector=0),
    "dependencies": Scenario("inspector_dependencies.png", inspector=2),
    "compilation": Scenario("inspector_compilation.png", inspector=3),
    "compare": Scenario("source_generated_compare.png", focus="post", view=2),
    "timeline": Scenario("timeline.png", frame=48),
}


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
    click_view(window, scenario.view)
    target = required(window, QComboBox, "WorkspaceGeneratedTarget")
    target_index = target.findText(scenario.target)
    if target_index >= 0:
        target.setCurrentIndex(target_index)

    # Drive the real timeline controls. Child order is stable: start, end, then FPS.
    timeline = required(window, QWidget, "Timeline")
    spins = timeline.findChildren(QWidget)
    numeric = [w for w in spins if w.metaObject().className() == "QDoubleSpinBox"]
    if len(numeric) != 3:
        raise RuntimeError("timeline range/FPS controls are incomplete")
    numeric[0].setProperty("value", 0.0)
    numeric[1].setProperty("value", 240.0)
    numeric[2].setProperty("value", 24.0)
    scrubber = required(timeline, QWidget, "TimelineScrubber")
    scrubber.setProperty("value", scenario.frame)

    # Stabilize the authored/generated and root layout proportions.
    for splitter in window.findChildren(QSplitter):
        if splitter.count() == 2:
            width = max(splitter.width(), 600)
            splitter.setSizes([int(width * 0.72), int(width * 0.28)])


def validate(window: WorkbenchWindow, scenario: Scenario) -> None:
    expected = {
        "scene": window.sceneDocument(),
        "post": window.document(),
        "shader": window.shaderToyDocument(),
    }[scenario.focus]
    checks = {
        "active tool": window.activeTool() == scenario.tool,
        "focused document": window.focusedDocument() == expected,
        "inspector tab": required(window, QTabWidget, "ActiveDocumentInspector").currentIndex()
        == scenario.inspector,
        "timeline visible": required(window, QWidget, "Timeline").isVisible(),
        "tool surface visible": required(window, QWidget, "ToolSurface").isVisible(),
        "viewport has size": window.sceneViewport().width() > 0
        if scenario.tool == "render-toy"
        else window.shaderToyViewport().width() > 0,
        "source editor populated": bool(
            required(window, QPlainTextEdit, "WorkspaceSourceEditor").toPlainText()
        ),
    }
    failed = [name for name, passed in checks.items() if not passed]
    if failed:
        raise RuntimeError("scenario semantic validation failed: " + ", ".join(failed))


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
            validate(window, scenario)
            output.parent.mkdir(parents=True, exist_ok=True)
            result["ok"] = window.grab().save(str(output), "PNG") and output.stat().st_size > 0
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
        code = capture_one(name, output, args.settle_ms)
        if code:
            return code
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
