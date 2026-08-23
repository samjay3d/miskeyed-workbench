"""Kit lifecycle adapter for the native Workbench Python binding."""

from __future__ import annotations

from importlib.metadata import version

import carb
import omni.ext
from miskeyed.workbench import DependencyGraph, NodeKind


class WorkbenchCoreExtension(omni.ext.IExt):
    """Own one native smoke-test object for exactly the Kit extension lifetime."""

    def on_startup(self, ext_id: str) -> None:
        self._ext_id = ext_id
        self._graph = DependencyGraph()
        node_id = self._graph.ensureNode("kit:adapter-smoke", NodeKind.Source)
        self.status = {
            "extension_id": ext_id,
            "workbench_version": version("miskeyed-workbench"),
            "native_node_id": int(node_id),
            "native_node_count": self._graph.nodeCount(),
        }
        carb.log_info(
            f"Miskeyed Workbench {self.status['workbench_version']} loaded; "
            f"native DependencyGraph call returned node {node_id}"
        )

    def on_shutdown(self) -> None:
        # Dropping the final Python owner lets Shiboken destroy the native QObject before
        # Kit tears down Python. The adapter never owns a Qt window or graphics device.
        self.status = {}
        self._graph = None
        self._ext_id = ""
