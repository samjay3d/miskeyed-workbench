# Third-party software and asset licenses

Miskeyed Workbench's own source is MIT-licensed; see [`LICENSE`](LICENSE). The following
projects are separate works governed by their own license files. This inventory records
architecture/deployment status and is not a substitute for those license texts.

| Project | Workbench status | Upstream license / notice |
| --- | --- | --- |
| Qt 6.8 | Required native UI and QRhi dependency | Commercial or LGPLv3/GPLv3 terms; see [Qt licensing](https://www.qt.io/licensing/) and the license files in the installed Qt SDK. |
| PySide6 / Shiboken6 | Required Python exposure/build dependency | LGPLv3/GPLv3 with Qt for Python exceptions; see [Qt for Python licensing](https://doc.qt.io/qtforpython-6/licenses.html). |
| Slang | Required compiler/runtime dependency | Apache-2.0 with LLVM exception; see [Slang LICENSE](https://github.com/shader-slang/slang/blob/master/LICENSE). |
| ANARI SDK | Optional research backend | Apache-2.0; see [ANARI-SDK LICENSE](https://github.com/KhronosGroup/ANARI-SDK/blob/next_release/LICENSE). An ANARI implementation may carry additional notices. |
| OpenUSD | Planned native scene-authority dependency; not yet linked or bundled | Modified Apache-2.0; see [OpenUSD LICENSE](https://github.com/PixarAnimationStudios/OpenUSD/blob/dev/LICENSE.txt). |
| MaterialX | Research probe dependency and planned native material/generation edge; not bundled | Apache-2.0; see [MaterialX LICENSE](https://github.com/AcademySoftwareFoundation/MaterialX/blob/main/LICENSE). |
| USD Working Group assets | Test/research-only pinned Git submodule; never wheel/runtime content | Apache-2.0 at [pinned upstream LICENSE](https://github.com/usd-wg/assets/blob/3b75c2dad6a494897557dcca0098257bcf42a8c6/LICENSE) and the local `third_party/usd-wg-assets/LICENSE` after submodule checkout. Individual asset notices remain authoritative. |

## USD test-assets submodule

The `usd-wg/assets` repository is pinned at
`3b75c2dad6a494897557dcca0098257bcf42a8c6`. Clone it only for explicit USD research or
compatibility work:

```text
git submodule update --init --depth 1 third_party/usd-wg-assets
```

Normal builds, wheels, source distributions, and the current runtime do not require or
package the submodule. Before selecting an asset for an automated fixture, record its
own notice, size, resolver requirements, and expected OpenUSD/MaterialX capabilities.
