# Native tests

The core is intended to be tested against the exact Qt 6.8 + Slang SDK used by the host.
Suggested first tests:

1. BLAKE2b vectors against Python `hashlib.blake2b(digest_size=32)`.
2. DependencyGraph: value-only dirties do not propagate ShaderDirty.
3. DependencyGraph: a shared dependency invalidates both downstream branches.
4. SlangCompiler: adding/removing a global numeric parameter changes reflection digest.
5. ShaderParameterModel: compatible values survive descriptor refresh.
6. SlangRhiWidget: a parameter edit uploads uniforms without rebuilding the pipeline.
