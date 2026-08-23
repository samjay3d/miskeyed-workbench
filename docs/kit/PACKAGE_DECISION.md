# Package and extension decision

## Decision

Workbench will not implement a general extension resolver for its Kit host. Kit owns extension
discovery, dependencies, enable/disable, ordering, metadata, extension paths, application
composition, and registry distribution. The repository contains two deliberately small
extensions: `miskeyed.workbench.slang_tools` declares a versioned dependency on
`miskeyed.workbench.core`.

This decision is scoped to the Kit host. Standalone Workbench remains a native/Python package
and has no dependency on Kit.

## Critical experiment answers

1. **Standalone extension need today:** none is demonstrated by this branch.
2. **Backend/device registration:** future ANARI devices are a narrow backend capability, not a
   reason for arbitrary package loading.
3. **Native registration:** keep it a narrow native contract if and when multiple concrete
   backends require it.
4. **Python tools:** normal Python packages remain sufficient outside Kit.
5. **Omniverse tools:** use Kit extensions, as demonstrated by the two manifests.
6. **Third-party dependency graph:** no Workbench-owned use case exists, so no resolver is added.

## Host comparison

| Responsibility | Shared Workbench capability | Qt standalone host | Kit host |
|---|---|---|---|
| Slang compile/reflection | `ShaderDocument`/`SlangCompiler` | consumes | consumes through binding |
| Parameter values/invalidation | model + `DependencyGraph` | consumes | consumes; value edit avoids compile |
| Graphics lifecycle | none | Qt/QRhi | Kit renderer (current decision) |
| Windows and panels | none | `WorkbenchWindow` | `omni.ui.Window` adapter |
| Extension discovery/resolution | none | no demonstrated need | Kit extension manager |
| Package delivery | native wheel/application package | pip/uv/native package | host registry/Kit package plus compatible Workbench native artifact |
| USD scene ownership | future host-neutral Hydra seam | future USD host | Kit active stage; never mirrored |

## What the repository proves versus defers

Static baseline tests prove the adapters are isolated from `cpp/`, `bindings/`, and the base
Python package, and that the tools manifest declares the core dependency. The optional lifecycle
test describes the enable/disable smoke test but is not run when Kit is absent. Registry
publication, packaging, `.kit` application composition, active-stage propagation, and viewport
interop remain deferred evidence—not claimed successes. Accordingly, no `.kit` application is
added yet; the plan requires working extensions before application composition.

NVIDIA describes `.kit` files as application experience configuration assembled from extension
dependencies, and the extension registry as the host's discovery/distribution mechanism:
[Kit application configuration](https://docs.omniverse.nvidia.com/kit/docs/kit-manual/latest/guide/creating_kit_apps.html),
[extension registry](https://docs.omniverse.nvidia.com/kit/docs/kit-manual/latest/guide/extensions_advanced.html#extension-registry).

## Distribution stories

```text
Standalone: pip/uv or native package -> Qt Workbench -> QRhi
Omniverse:  Kit application/registry -> Workbench Kit extensions -> same native binding
```

The package boundary follows its host. A future thin `.kit` app should be only a dependency list
after runtime and registry tests pass; it must not become a second application framework.
