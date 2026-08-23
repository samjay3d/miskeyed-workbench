"""Artist-facing Kit UI backed by Workbench's native ShaderDocument."""

from __future__ import annotations

from pathlib import Path

import omni.ext
import omni.ui as ui
from miskeyed.workbench import ShaderDocument, ShaderParameterModel
from PySide6.QtCore import QUrl


class SlangInspectorExtension(omni.ext.IExt):
    WINDOW_TITLE = "Miskeyed Slang Inspector"

    def on_startup(self, ext_id: str) -> None:
        self._document = ShaderDocument()
        self._document.setLive(False)
        self._path = ui.SimpleStringModel("")
        self._source = ui.SimpleStringModel("")
        self._diagnostics = ui.SimpleStringModel("Load a .slang file to inspect it.")
        self._window = ui.Window(self.WINDOW_TITLE, width=560, height=720)
        self._window.set_visibility_changed_fn(self._visibility_changed)
        self._build_ui()

    def _build_ui(self) -> None:
        with self._window.frame:
            with ui.VStack(spacing=6):
                ui.Label("Slang source")
                ui.StringField(self._path)
                with ui.HStack(height=28):
                    ui.Button("Load + Compile", clicked_fn=self._load_and_compile)
                    ui.Button("Compile Editor", clicked_fn=self._compile_source)
                ui.StringField(self._source, multiline=True, height=260)
                ui.Label("Diagnostics")
                ui.StringField(self._diagnostics, multiline=True, read_only=True, height=100)
                ui.Label("Reflected Parameters")
                self._parameters = ui.VStack(spacing=4)

    def _load_and_compile(self) -> None:
        path = Path(self._path.as_string)
        self._document.setFileUrl(QUrl.fromLocalFile(str(path)))
        if not self._document.load():
            self._diagnostics.set_value(self._document.diagnostics())
            return
        self._source.set_value(self._document.source())
        self._compile_source()

    def _compile_source(self) -> None:
        self._document.setSource(self._source.as_string)
        self._document.compile()
        self._diagnostics.set_value(self._document.diagnostics() or "Compile succeeded.")
        self._rebuild_parameters()

    def _rebuild_parameters(self) -> None:
        model = self._document.parameters()
        role = ShaderParameterModel.Role
        self._parameters.clear()
        with self._parameters:
            for row in range(model.rowCount()):
                index = model.index(row, 0)
                name = str(model.data(index, role.NameRole))
                label = str(model.data(index, role.LabelRole) or name)
                value = model.value(name)
                with ui.HStack(height=24):
                    ui.Label(label, width=220)
                    field_model = ui.SimpleStringModel(str(value))
                    field_model.add_end_edit_fn(
                        lambda current, parameter=name: self._set_parameter(
                            parameter, current.as_string
                        )
                    )
                    ui.StringField(field_model)

    def _set_parameter(self, name: str, value: str) -> None:
        # ShaderParameterModel updates the packed value node; ShaderDocument.compile()
        # is deliberately not called for value-only edits.
        if not self._document.parameters().setValue(name, value):
            self._diagnostics.set_value(f"Value rejected for {name}: {value}")

    def _visibility_changed(self, visible: bool) -> None:
        if not visible:
            self._window = None

    def on_shutdown(self) -> None:
        self._window = None
        self._document = None
