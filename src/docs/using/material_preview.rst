USD assets in Render Toy
========================

Choose **Open > Open USD asset in Render Toy...** and select ``asset.usda``,
``asset.usdc``, or ``asset.usd``. The current bounded adapter looks beside it for
``asset.preview.slang``, opens that derived scene representation as a read-only
``ShaderDocument``, and binds it to Render Toy's **Scene** pass. The existing editable
**Post** shader still controls the final Render Toy viewport. Reopening the USD asset
reloads and recompiles the complete derived scene shader: this intentionally obvious
whole-cache break precedes incremental USD/MaterialX notices.

The fixture at ``spikes/usd_materialx_probe/fixture/root.usda`` includes
``root.preview.slang`` as the controlled base-color/roughness example. Compilation
diagnostics and generated backend code remain inspectable, but generated source cannot
be edited or saved through Workbench.

This is not yet general USD rendering. Workbench does not parse or traverse the selected
stage, and the sidecar is not a second material authority. Loading Houdini-authored USD
and MaterialX directly requires the next native OpenUSD/MaterialX integration slice. The
``usd-wg/assets`` test-only submodule supplies larger compatibility cases without becoming
packaged application content.
