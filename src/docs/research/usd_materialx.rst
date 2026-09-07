USD and MaterialX authoring evaluation
======================================

Status and decision
-------------------

This record evaluates the next authoring milestone. Only the bounded derived-sidecar
Material Preview entry point described below is shipped; native USD traversal and material
authoring remain future work. The source-review baseline is ``main`` at
``f9f0e9b9f24f166100e18233ea28db7e92817459`` (7 September 2026). The checked-out
tree matched that revision when the evaluation began.

The decision is to keep three authored destinations and one derived product explicit:

* a native future ``MaterialDocument`` owns a shared ``.mtlx`` graph and references
  separately authored Slang implementation modules;
* a native future USD stage document owns the stage and selects a writable look layer;
  assignments and asset-local exceptions are USD opinions in that layer;
* custom Slang remains authored source in its own file; and
* MaterialX-generated Slang, compiler reflection, backend code, and ``QShader`` packages
  are replaceable derived products. A preview session consumes them but never saves
  them as material truth.

Therefore a parameter control must display its destination. **Edit MaterialX** changes
the shared graph; **Override in Look** authors the selected USD layer; **Edit
Implementation** changes a Slang module. Save writes only that destination, revert
reloads it, and undo records the destination with the command. Preview-only uniform
changes are labelled transient and cannot satisfy the save/reopen authoring contract.

Fixture and representation
--------------------------

``spikes/usd_materialx_probe`` contains one quad with positions, normals and ``st`` UVs,
one look layer, a MaterialX standard-surface graph, a 2x2 PPM texture, and a probe. The
fixture assumes right-handed Y-up geometry, object-space normals transformed by the
inverse-transpose, ``st`` origin at the lower left, linear working values, and an sRGB
base-color texture decoded to linear before shading. Roughness is linear and scalar.

The chosen representation is a MaterialX document composed into USD through OpenUSD's
``usdMtlx`` file-format integration, with the mesh binding and stronger parameter
opinions in ``look.usda``. It keeps the portable graph independently editable while
allowing non-destructive asset-local USD opinions. Re-authoring a parallel UsdShade
network was rejected because it would create two graph authorities. This choice is
**provisional until the pinned native USD build loads, edits, and reopens the fixture**.
The probe deliberately fails with an exact missing-module message rather than treating
XML parsing or a Python import as integration proof.

Run from the repository root::

   .\.venv\Scripts\python.exe spikes/usd_materialx_probe/probe.py check
   .\.venv\Scripts\python.exe spikes/usd_materialx_probe/probe.py usd-roundtrip
   .\.venv\Scripts\python.exe spikes/usd_materialx_probe/probe.py generate

``check`` is dependency-free. ``usd-roundtrip`` requires OpenUSD Python bindings and
uses a fresh child process for reopen validation. ``generate`` requires the upstream
MaterialX core, generator, and ``PyMaterialXGenSlang`` bindings. Generated vertex and
pixel sources are written under a caller-selected output directory, never over the
fixture. A future native draw adapter must then compile those exact files and record
entry points, vertex inputs, uniforms, textures, samplers and host-provided values; a
hand-written substitute is not acceptable evidence.

Evidence ledger
---------------

.. list-table:: Evidence obtained in this evaluation
   :header-rows: 1
   :widths: 19 18 38 25

   * - Level
     - Result
     - Evidence
     - Consequence
   * - Source review
     - Observed
     - ``ShaderWorkspace`` owns shader documents and shared time; sessions own bindings.
       ``CompileResult`` embeds ``QShader`` entry points; ``compileProgram`` generates
       HLSL, GLSL, SPIR-V, Metal source and Qt packages from one Slang program.
     - Preserve session ownership; do not put USD inside ``ShaderDocument``.
   * - Fixture structure
     - Checked
     - The dependency-free probe validates XML, texture bytes, relative paths, mesh
       normals/UVs, assignment and declared edit targets.
     - The same asset is the acceptance fixture for following PRs.
   * - Native dependency build
     - Blocked here
     - No ``pxr`` module, native SDKs, plugins, or data libraries are installed. The
       MaterialX Python wheel used for generation is not native-link/deployment evidence.
     - Native target names and deployment layout remain provisional.
   * - Generation / reflection
     - Generation passed; compile blocked
     - The PyPI ``MaterialX==1.39.5`` wheel validated the graph and generated preserved
       ``vertexMain`` and ``fragmentMain`` sources (7,355 and 89,392 normalized bytes). Inputs include
       position, normal, tangent and UV; resources include vertex/pixel constant buffers,
       environment/light host values and combined texture/samplers. No ``slangc`` or
       Workbench native build is present, so compilation/reflection was not run.
     - PR 1 must compile the preserved sources and capture exact diagnostics/layout.
   * - Actual draw
     - Not run
     - Generated source exists, but no compiled program or supported desktop QRhi
       runtime is available here.
     - Do not claim the current global-uniform/fullscreen assumptions support it.
   * - USD persistence
     - Not run
     - ``pxr`` and the ``usdMtlx`` plugin are absent.
     - Representation remains provisional until fresh-process reopen passes.
   * - Installed package
     - Not applicable
     - This PR changes research assets and documentation only.
     - A future wheel must prove plugins/data and native linkage independently.

Ownership map
-------------

.. list-table:: Current versus proposed boundaries
   :header-rows: 1
   :widths: 18 13 31 38

   * - Component
     - Action
     - Owner and lifetime
     - Consumer / change
   * - ``ShaderWorkspace`` and tool sessions
     - Adapt later
     - Workspace owns shader documents and evaluation objects; sessions own bindings.
     - Add the smallest application composition only when material and stage documents
       exist; do not turn this shader-specific class into a generic registry now.
   * - ``ShaderDocument``
     - Keep
     - Owns authored Slang and current compile products.
     - Remains independent; custom implementation files may be explicit inputs.
   * - Slang compiler product
     - Adapt
     - A future owning result keeps Slang component/session/blob lifetimes valid and
       associates reflection and layout with the target they describe.
     - Consumers inspect generated bytes before an optional QRhi packager creates
       ``QShader``. Qt strings/containers may remain value types.
   * - MaterialX graph/generator
     - Delegate upstream
     - ``MaterialDocument`` owns file/save state; MaterialX owns graph semantics and
       generation.
     - Preview and later USD workflows consume graph identity and generated stages.
   * - USD stage/look layer
     - Delegate upstream
     - A native stage document owns ``UsdStage`` and explicit edit target; USD owns
       composition and authored meaning.
     - Selection/editor author opinions; Hydra consumes scene changes.
   * - Preview
     - Adapt
     - A fixture-specific session owns bindings and QRhi resources with deferred
       retirement.
     - Controlled mesh first; general USD scenes go through Hydra, not a second extractor.
   * - Time
     - Keep
     - ``TimeTransport`` evaluates shared ``TimeContext``.
     - Adapters apply a sample to uniforms or USD evaluation without recompiling.
   * - ANARI foundation
     - Defer/reuse
     - Existing host owns library/device lifecycle; implementations own render resources.
     - Connect only after the authoring path works without ANARI.
   * - Python
     - Keep at edge
     - Shiboken exposes stable native objects.
     - Python probe is research only; no Python-owned production stage or renderer.

Invalidation contract
---------------------

Identity answers whether a product is the same; dirty work states what must happen.
Workbench observes USD/MaterialX notices and identities without reproducing either
dependency graph.

.. list-table:: Minimum required work
   :header-rows: 1
   :widths: 24 46 30

   * - Change
     - Required work
     - Must not happen
   * - Material value
     - Save to MaterialX or USD override as selected; update parameter bytes and redraw.
       Regenerate only if upstream marks the value compile-time.
     - No implicit second material record.
   * - Graph topology
     - Regenerate stages, compile affected targets, compare layout/pipeline identities,
       rebuild only changed bindings/pipeline, redraw.
     - Do not overwrite authored extensions.
   * - Slang implementation
     - Recompose through supported generator/include APIs, compile dependants and map
       diagnostics to the module and graph implementation.
     - Never patch generated text as the persistence mechanism.
   * - UI metadata
     - Rebuild editor schema only; compile solely when metadata is embedded in generated
       source and record that observed limitation.
     - Do not promise annotation-only pipeline reuse before it is measured.
   * - Texture bytes/path
     - Resolve relative to the owning asset, reload the texture, update bindings, redraw;
       regenerate only if topology/type changed.
     - No shader compile for ordinary pixel changes.
   * - Time sample
     - Re-evaluate USD or upload host uniforms and redraw.
     - No source/hash change or compile.
   * - Renderer/device
     - Rebuild consumer/device-side state from the same authored documents.
     - No change to USD/MaterialX ownership.

Compiler and QRhi decision
--------------------------

The smallest follow-up extraction is an owning, target-specific Slang compilation
product containing entry-point identity, stage, generated blob and its resource/layout
snapshot. The owner retains the Slang session/component objects required by reflection,
or copies all reflection into value records before releasing them; raw reflection
pointers never escape. A separate QRhi packager consumes one compatible target and
adds ``QShader``. ``CompiledEntryPoint`` and ``ShaderDocument`` are current exported
and Shiboken-visible contracts, so the split is an intentional API/ABI change with no
compatibility shim and needs native plus Python equivalence tests.

Pinned experiment envelope
--------------------------

The first rerun should pin **OpenUSD 26.08** (tag
``cb5613f6da7c61b56fe86dbe8cc1cbe9f0d84ef1``), **MaterialX 1.39.5** (tag
``7b64921ef1d42f2d57871e9d2c43dc11f041f26b``), the repository's proven **Slang
2026.14** line (tag ``c4bc8a6fa65cf90f4304b2194fe7aaea8e18b236``), and **Qt 6.8.3**.
This is a proposed compatibility envelope, not a successful build claim. Build one
MaterialX and link both OpenUSD and the probe to it; require ``MaterialXCore``,
``MaterialXFormat``, ``MaterialXGenShader``, ``MaterialXGenHw`` and
``MaterialXGenSlang``. Verify actual exported target spellings from installed package
configs before changing Workbench CMake. For USD verify ``usd``, ``usdShade``,
``usdMtlx``, ``hd``, ``hdMtlx`` and the chosen imaging plugin. Record plugin/resource
search paths, MaterialX libraries, texture paths and DLL/shared-library loading in a
fresh installed environment. A successful Python import proves none of those native
deployment properties.

Ordered implementation PRs
--------------------------

Each task stops rather than generalizing when its observable fixture contract passes.

1. **Compiler-consumer seam.** Problem: compiler output is inseparable from ``QShader``.
   Add the owning target-associated product and optional QRhi packaging, then address
   only fixture-demonstrated input/resource gaps. Likely files are ``SlangCompiler.*``,
   ``Qt68ShaderBridge.*``, ``ShaderDocument.*``, bindings and compiler/render contracts.
   Public ABI and Shiboken change intentionally. Validate real upstream vertex/pixel
   generation, diagnostics, reflection and one QRhi draw while existing toys and
   deferred resource retirement pass. Stop after a non-viewport consumer can inspect
   the product and the controlled fixture draws.
2. **USD asset and look editing.** Problem: there is no native stage owner or edit
   target. Add only the stage document/session composition needed to open the fixture,
   select its mesh/material, bind it, author an override to ``look.usda``, save, and
   reopen in a fresh process. Likely files are a new device-neutral ``usd`` directory,
   shell composition, optional CMake discovery, tests and deliberate bindings. Validate
   original asset bytes and relative references remain unchanged. Stop at persistence;
   do not add general scene rendering.
3. **MaterialX authoring and controlled preview.** Add a native material document for a
   small supported node/input set, preserved extension-module references, upstream
   generation and a fixture preview session. Likely files are a new ``material`` edge,
   tool contribution, Inspector adapter, CMake/bindings and fixture contracts. Validate
   graph reopen, generated-stage diagnostics, texture/color assumptions and value
   destination. Stop when the controlled surface reproduces the saved graph.
4. **Connected material workflow.** Resolve a selected USD surface to the composed
   MaterialX asset/override and use established USD imaging/Hydra integration for the
   supported scene path. Validate selection, edit, preview, save and reopen from the
   same stage. Stop at one explicit imaging path; do not build a private USD extractor.
5. **ANARI scene consumer.** Reuse the optional host and connect OpenUSD/Hydra/hdAnari
   to one selected device. The initial subset is USD Preview Surface base color,
   roughness and one UV base-color texture on Helide; MaterialX graphs must translate to
   that subset or report unsupported/approximated nodes. Validate one scene delta and
   frame. Stop before multi-device comparison or arbitrary Slang execution.

Scene-preview strategy
----------------------

The starting Material Preview uses a controlled fullscreen adapter. PR 3 replaces it
with a controlled QRhi mesh to establish authoring and generated-Slang behavior.
PR 4 uses USD imaging/Hydra for a general selected surface. Render Toy remains a shader
test stage, not a USD renderer. PR 5 may add hdAnari as another Hydra consumer, but the
authoring loop remains useful without it. A baked procedural texture is reported as a
bake and is not equivalent to a live custom BSDF.

Unresolved evidence and non-goals
---------------------------------

The pinned native combination, ``usdMtlx`` composition path, generated stage entry
names, complete host-resource contract, source mapping, QRhi draw, and installed plugin
layout remain unresolved until the recorded probes run. The next PR must run generation
first and may amend the compiler seam based on that output.

This roadmap deliberately does not generalize document storage, plugins, dependency
injection, execution graphs, scene extraction, renderers, or frame interop. Painter-like
brushing/layers, ECS/Zig, SHADERed, Kit/WASM, procedural variants not needed by the
fixture, a new renderer, multi-device comparison, zero-copy frames, and arbitrary Slang
materials across ANARI devices remain deferred.

See :doc:`../architecture/overview` for shipped architecture and :doc:`anari` plus
``records/ANARI_HOST_IMPLEMENTATION_PLAN.md`` for the existing later ANARI work.
