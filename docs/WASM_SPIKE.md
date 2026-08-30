# Qt WebAssembly viability spike

**Status:** experimental-only. CI now performs the complete cross-build and publishes a
browser bundle for every relevant pull request and manual run. This is deliberately a
narrow build edge, not a fourth supported platform.

## Download and run the CI artifact

Open the **WebAssembly spike** Actions run, download
`miskeyed-workbench-wasm-spike`, extract it, and serve the extracted directory:

```sh
python3 serve.py
```

Then open `http://127.0.0.1:8000/`. Do not open `index.html` directly: browsers do not
provide the required Wasm fetch behavior from a `file:` URL. `SHA256SUMS.txt` records
the exact files built by CI. The workflow retains each artifact for 14 days.

The workflow pins Qt, Emscripten, and Slang, builds Slang's native code generators,
cross-compiles the static Slang library with the same Emscripten ABI as Qt, builds the
Workbench target, and uploads the resulting HTML/JavaScript/Wasm bundle. A failed
cross-build is therefore a CI failure rather than an untested instruction in this report.

## What the prototype contains

With an Emscripten Qt 6.8 kit, configure the repository itself rather than a parallel
application:

```sh
qt-cmake -S . -B build-wasm \
  -DMISKEYED_WORKBENCH_WASM_SPIKE=ON \
  -DMISKEYED_WORKBENCH_BUILD_PYTHON=OFF \
  -DBUILD_TESTING=OFF \
  -DSLANG_ROOT=/path/to/wasm-slang-sdk
cmake --build build-wasm
```

The Wasm branch builds only the existing core, dependency graph, workspace/document,
in-process compiler/reflection, time transport, and single-pass `QRhiWidget` sources.
The small browser shell opens the packaged default shader in `ShaderWorkspace`, binds
it to `TimeContext`, and requests a fullscreen pass. Python/Shiboken, the desktop
editor/LSP, Render Toy shell, and ANARI are not linked. The same C++ objects remain in
charge; there is no JavaScript application or embedded JS engine.

## Local and CI verification

The repository workflow is the authoritative reproducible build because the ordinary
development container does not contain Qt's Wasm SDK. It uses Qt 6.8.3, Emscripten
3.1.56 (Qt's matching toolchain), and Slang 2026.14.1. Artifact assembly fails if the
expected `.html`, `.js`, or `.wasm` file is absent and prints the size of every shipped
file in the Actions log.

CI proves configuration, compilation, and linking. A downloaded artifact still needs
interactive browser validation for rendering, input, and context restoration; CI does
not turn those behavioral questions into confirmed facts.

### Confirmed source-level separations

* Python discovery was unconditional. It is now restricted to binding builds and is
  forcibly disabled for Emscripten.
* The monolithic native library pulled in editor, LSP, Render Toy, and desktop shell
  code. The browser target now selects the concrete vertical-slice source set without
  inventing another document or render core.
* `SlangRhiWidget` selected Vulkan for every non-Windows/non-macOS host. Wasm now
  explicitly selects Qt's OpenGLES2 QRhi API.
* Authored source is a Qt resource and has a `workbench:` identity, so the first slice
  does not depend on native file dialogs or persistent local paths.

## Blocking questions

### Slang link and code generation

The current SDK contract finds a prebuilt `slang` library. A native Slang SDK cannot
be linked into an Emscripten target, so CI builds Slang's generators natively and then
cross-compiles and installs a static Slang SDK using the same Emscripten toolchain as
Qt. This directly tests the C++ API used by Workbench; it does not use Slang's optional
JavaScript bindings or move compilation ownership into JavaScript.

Even after linking, the live `QShader` package currently contains SPIR-V 1.5, HLSL
SM5, and Metal 2 output. GLSL 450 is generated only for display. Qt's WebGL path needs
a representation accepted by its OpenGL ES backend (and WebGL 2 ultimately needs GLSL
ES 3.00 constraints). Do not silently label the existing desktop GLSL as browser-ready:
the next probe must establish a Slang GLSL-ES target and add it to `QShader`, then test
uniform reflection/binding equivalence. This is the immediate rendering blocker.

### Qt and browser behavior (unverified)

`QApplication`, `QMainWindow`, `QSplitter`, `QPlainTextEdit`, and `QRhiWidget` are the
only visible widgets in the spike. Editing, focus, resize, keyboard input, clipboard,
IME, canvas sizing, and context-loss recovery require browser tests. Native file
dialogs and filesystem watching are intentionally outside the first shell; packaged
resources and in-memory URLs cover its only document.

Qt private QRhi and private shader-description APIs remain a version-coupled risk.
They are already part of the desktop architecture rather than a Wasm-specific
abstraction, but the Qt Wasm kit must actually ship the matching private headers.

### Threads, startup, and size (unmeasured)

The slice makes no pthread assumption and performs compilation synchronously today.
That is architecturally coherent but may block the browser main thread. Measure compile
latency first; only then decide whether a pthread-enabled Wasm build or cooperative job
edge is justified. Slang compiler size and startup cost may independently make local
compilation unacceptable.

The build enables growing linear memory with a 1 GiB ceiling as a diagnostic default,
not a product budget. This must be tested against browser cross-origin isolation and
32-bit Wasm address-space behavior before choosing a real cap.

## Memory accounting plan

Workbench should report logical bytes held by document text, dependency/source blobs,
compiled-stage blobs, parameter/upload buffers, and live QRhi resource estimates.
Emscripten can additionally report linear-heap size and usage. Neither number is the
browser process resident set nor authoritative GPU allocation; those remain browser
and driver owned. Report these as three distinct categories and never infer physical
browser memory from the Workbench logical budget.

No memory-accounting API was added in this spike because no browser artifact exists to
validate it and a desktop-only estimate would create false authority.

## Cloud relevance

The in-memory `openSource(QUrl, name, source)` edge is already suitable for a later
remote source snapshot or delta consumer, and dependency identities remain independent
of host filesystem paths. That supports the proposed bounded client model in principle.
Networking, delta protocols, resource streaming, and caches are intentionally absent;
none is needed to answer the local compiler/render viability question.

## Recommendation and next experiment

Keep this **experimental-only**. Do not change desktop ownership or promise browser
support yet. On a machine with Qt 6.8 for WebAssembly:

1. download the CI artifact and verify one compile and reflection result in-browser;
2. produce GLSL ES suitable for Qt's WebGL QRhi backend and package it in `QShader`;
3. render the fullscreen triangle, then test resize and WebGL context restoration;
4. record `.wasm`/compressed transfer size, first-paint and first-compile time, linear
   heap high-water mark, logical cache bytes, and tested browser/Qt/Slang revisions.

Abandon the browser target if Slang cannot be built compatibly, if GLSL-ES cannot
preserve the reflected binding contract, or if compiler transfer/startup cost is not
acceptable. A cloud-connected thin client could omit local Slang later, but that would
answer a different architecture question and must not be used to disguise failure of
this vertical slice.
