# Kit viewport decision

## Provisional decision: C first, B as a future explicit contract

The spike intentionally has no viewport. For the first useful feature, Kit owns its window and
UI while Workbench supplies shader authoring, reflection, values, invalidation, and diagnostics.
This is option **C**: Kit owns scene rendering and Workbench supplies technical-art services.

This is a provisional architectural choice, not evidence that a production viewport integration
has been validated. No Kit SDK is available in baseline CI, and this branch has no USD/Hydra or
ANARI implementation to test.

## Options considered

### A. Embed `SlangRhiWidget`

Rejected for the spike. The widget assumes Qt owns the window, QRhi device, targets, command
buffers, and resource-retirement cadence. Embedding it would couple two UI and graphics
lifecycles and would test window plumbing rather than reusable Workbench capability. Reconsider
only if NVIDIA publishes and supports a native-window or external-texture contract that can
preserve both owners' lifetime rules.

### B. Export a frame/texture

Potentially useful, but no `FrameSnapshot` or cross-device synchronization contract exists.
Such a contract must specify API/device identity, ownership, synchronization, color space,
extent, and retirement. Do not introduce it until an artist-facing use case needs Workbench's
own rendered pixels inside Kit.

### C. Kit renders; Workbench supplies services

Selected for the current spike. It needs no QRhi bridge and demonstrates a native capability
through `ShaderDocument`. It also aligns with Kit's existing USD context rather than creating a
private scene.

## USD/Hydra/ANARI follow-up gate

The intended flow remains:

```text
Kit active USD stage (source of truth)
          |
          v
Hydra dirty propagation (single owner)
          |
          v
hdAnari -> ANARI device
```

A future Kit adapter may observe stage events and pass a stage/path handle into the planned
Hydra/ANARI seam. It must not copy the full stage, implement another dirty graph, or make
`ShaderDocument` own USD. NVIDIA's `omni.usd` API exposes the host USD context and stage event
stream; those are the integration edge to validate when the native scene seam exists:
[omni.usd API](https://docs.omniverse.nvidia.com/kit/docs/omni.usd/latest/omni.usd/omni.usd.UsdContext.html).

## Evidence required to revisit

* A supported Kit API for native image/texture consumption and its synchronization rules.
* A working host-neutral frame or Hydra/ANARI contract in Workbench.
* A lifecycle test covering resize, device loss, extension disable, and in-flight release.
* Proof that a small USD edit follows Kit/Hydra dirty propagation without reopening the stage.
