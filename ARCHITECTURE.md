# slang-qrhi architecture

`slang-qrhi` is a new native SDK. The old Python renderer/toolchain API is intentionally not part of this package.

## Ownership

- **Qt 6.8 / QRhi** owns the graphics device, render targets, command buffers, and widget lifecycle.
- **Slang C++ API** owns source compilation, module composition, linking, reflection, entry-point hashes, and backend shader generation.
- **QShader bridge** packages in-memory Slang output for QRhi. There are no runtime compiler subprocesses.
- **ShaderDocument** is the live authoring state and connects source, reflection, parameters, dependency state, and generated shader packages.
- **ShaderParameterModel** is the authoritative Qt model for reflected values. It backs both native Qt and Shiboken/PySide consumers.
- **DependencyGraph** is a shader-specific persistent Merkle DAG. Hashes answer identity; dirty flags answer work scheduling.
- **SlangRhiWidget** is the embeddable native viewport used by both the standalone executable and PySide6.

## Merkle DAG rule

A source edit does not make every semantic product a hash-child of source. Compiler outputs become independent semantic nodes after compilation.

For example, an implementation-only source edit may change `EntryPoint` while `ParameterLayout` retains the same digest. The viewport then rebuilds the shader/pipeline while the parameter inspector remains untouched.

```text
Source -> EntryPoint ---------------------> Pipeline
                 ^                           ^
                 |                           |
          Slang compile                ParameterLayout
                                            |      |
                                            v      v
                                        UiSchema  Values
```

This is deliberately different from a serialized snapshot Merkle tree: the graph is long-lived, shared dependencies form a DAG, dependency ordering is canonicalized, and invalidation semantics are node-kind specific.

## Runtime update classes

- parameter value: uniform buffer update only
- UI metadata: inspector/model update only
- texture/resource content: resource upload only
- reflected binding layout: bindings + pipeline rebuild
- shader implementation: compile + shader package + pipeline rebuild
- render state: pipeline rebuild only

The target UX is Substance/Houdini-like: edit shader declarations, let reflection generate controls, and keep ordinary control changes out of the compiler path.
