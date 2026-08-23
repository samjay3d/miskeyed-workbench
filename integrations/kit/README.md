# Optional Omniverse Kit integration

This directory is not part of the standalone build. It contains two Kit Python extensions that
consume an already-installed, ABI-compatible `miskeyed-workbench` native wheel:

* `miskeyed.workbench.core` proves lifecycle, version reporting, native loading, and a real
  `DependencyGraph` call.
* `miskeyed.workbench.slang_tools` depends on core and provides the first Slang Inspector panel.

Add the absolute `exts` directory to the host application's extension search paths and enable
`miskeyed.workbench.core` first. The adapter deliberately does not invoke pip or vendor a second
copy of Workbench; package installation belongs to deployment, not extension startup.

Run `kit_lifecycle_smoke.py` through Kit's async test runner to exercise disable/re-enable. It is
named so baseline pytest does not collect a module that necessarily imports the unavailable Kit
SDK.

See the decision documents in `docs/kit/` before extending this spike. In particular, do not add
Kit imports or linkage to `cpp/`, `bindings/`, or `python/miskeyed/workbench`.
