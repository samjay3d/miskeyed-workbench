"""Optional smoke test executed by Kit's Python, not baseline pytest.

Run with Kit's test runner after adding ``integrations/kit/exts`` as an extension path.
The test intentionally performs no viewport work.
"""

import omni.kit.app


async def test_workbench_extension_lifecycle():
    manager = omni.kit.app.get_app().get_extension_manager()
    extension_id = manager.get_enabled_extension_id("miskeyed.workbench.core")
    assert extension_id
    assert manager.set_extension_enabled_immediate(extension_id, False)
    assert manager.set_extension_enabled_immediate(extension_id, True)
