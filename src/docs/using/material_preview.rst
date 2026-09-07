Material Preview starting point
===============================

Material Preview establishes one narrow consumer edge before native USD stage and
MaterialX document ownership land. Choose **Open > Open USD material preview...** and
select ``asset.usda``, ``asset.usdc``, or ``asset.usd``. Workbench looks beside it for
``asset.preview.slang``, opens that derived source as a ``ShaderDocument``, compiles it,
and binds it to the Material Preview viewport.

The sidecar must contain vertex and fragment entry points. The fixture at
``spikes/usd_materialx_probe/fixture/root.usda`` includes
``root.preview.slang`` as a minimal example. A generator or adapter may replace this
file without changing the USD asset. Compilation errors remain attached to the derived
shader document and are visible in the existing Inspector.

This boundary is intentionally honest about its limits:

* Workbench does not parse, traverse, edit, or save the selected USD stage yet.
* The sidecar is a derived preview representation, not a second material authority.
* Inspector uniform changes are transient preview adjustments; they are not USD or
  MaterialX authoring.
* Relative texture/resource binding and generated MaterialX stages remain the next
  compiler-consumer implementation step.

The first complete artist loop still requires the native stage/look owner and
MaterialX document described in :doc:`../research/usd_materialx`.
