# ANARI backend compatibility lock

This file records the upstream surface used to implement the optional ANARI backend.
It is deliberately separate from the shipped Shader Toy and Render Toy dependency set.

## Reviewed upstream

| Component | Reviewed source | Backend use |
| --- | --- | --- |
| ANARI-SDK | `7897cbe425e5000ce890631bc97d14e00c750175` from `next_release` | Frontend ABI, loader, CMake target, Helide/debug tooling |
| TSD | VisRTX `next_release/tsd` | Candidate-management and capture-device reference only |
| VisRTX | `next_release` | Optional interactive device |
| Island | Research snapshot only | Future independent ANARI device |

The implementation consumes the exported `anari::anari` CMake target and public
`<anari/anari.h>` API. It does not copy ANARI headers or loader behavior into this
repository.

## Runtime contract

- `SLANG_QRHI_WITH_ANARI=OFF` is the default and does not call `find_package(anari)`.
- With the option enabled, the selected ANARI installation must export
  `anari::anari`.
- Candidate strings are passed to `anariLoadLibrary()` unchanged. This preserves the
  SDK's ordinary name and explicit `name,path` forms.
- Candidate list entries are separated by semicolons in
  `MISKEYED_ANARI_LIBRARIES`; empty and duplicate entries are removed deterministically.
- ANARI libraries remain loaded until every device created from them is released.

## Validation status

The standalone probe configures and builds against an installed ANARI-SDK without Qt or
Slang. Its candidate test passes, and the `sink` SDK device has been loaded, queried,
created, committed, and released through the probe. A full Windows runtime lock still
requires the production Qt/Slang/OpenUSD environment. Helide is the first rendering
smoke device; VisRTX remains capability-gated.

The `ANARI backend` CI job repeats that standalone build and sink-device lifecycle on
Linux for every backend-affecting pull-request change. The existing Windows test matrix
continues to exercise the default-off Workbench path independently.

Update this file with exact release/commit IDs and runtime artifacts when that Windows
build is established. Do not turn an experimental backend dependency into a required
dependency of `slang_qrhi_core`.
