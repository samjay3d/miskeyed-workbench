Real geometry before USD
========================

Render Toy's base Scene sample is now a six-vertex, UV-mapped material card rather
than a fullscreen SDF raymarch. Slang owns the vertex positions, UVs, normal, and
material evaluation. The QRhi host owns only the selected vertex count, command
submission, and resource lifetime. The base Scene source is marked ``[generated]`` and
is read-only; the Post shader remains editable and controls the final viewport.

Choose **Samples > Scene — SDF studio** to return to the previous raymarched SDF
scene. Keeping raster geometry and SDF as separate samples makes the next USD question
concrete: OpenUSD mesh topology and primvars must eventually produce the same explicit
scene draw product instead of being converted into an SDF or a hand-written sidecar.

What USD data must supply
-------------------------

The next native slice must open a stage and select a mesh, then preserve at least:

* points, face-vertex counts and indices;
* normals and interpolation;
* ``primvars:st`` values, indices and interpolation;
* transforms, orientation, subdivision policy and material binding; and
* the composed MaterialX/UsdShade inputs and resolved texture asset paths.

That product will bind as Render Toy's Scene pass while the current Post pass remains
unchanged. A whole-stage identity change may rebuild the mesh/material product at first;
incremental USD notices come only after that visible path works. The
``usd-wg/assets`` submodule is test-only input for those compatibility checks and is not
packaged with Workbench.

Current limit
-------------

Workbench does not yet open Houdini-authored USD directly. The PyPI packages are useful
for the research probe, but they do not replace the native C++ ownership, plugin, and
deployment work required by the application. No ``.preview.slang`` convention is part of
the product contract.
