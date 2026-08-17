---
name: prepare-release
description: 'Prepare a miskeyed-workbench release (cut a version). Use when asked to "prepare a release", "cut a release", "bump the version", "release X.Y.Z", or "stamp the changelog". Bumps the version in pyproject.toml + CMakeLists.txt, dates the CHANGELOG [Unreleased] section, rebuilds, runs the headless tests, and commits — without tagging or pushing.'
argument-hint: 'the version to cut, e.g. 0.2.0'
---

# Prepare Release

Cuts a new version of **miskeyed-workbench** by applying the mechanical release edits,
verifying the build, and committing. It deliberately stops before tagging/pushing so a
human merges and triggers publishing.

## When to Use

- "prepare a release", "cut a release 0.2.0", "bump the version", "stamp the changelog".
- Any time the `## [Unreleased]` changelog section is ready to ship.

## Do NOT

- Do NOT `git tag`, `git push`, or trigger the publish workflow. Publishing happens after
  the human merges to `main` (the `release.yml` tag trigger does the PyPI upload).
- Do NOT invent a version. If the user didn't give one, pick it from semver + the
  `[Unreleased]` entries (new feature → minor, fixes only → patch) and confirm.

## Choose the Version

Inspect `## [Unreleased]` in [CHANGELOG.md](../../../CHANGELOG.md):

- New user-facing feature → bump the **minor** (`0.1.x` → `0.2.0`).
- Bug fixes / internal only → bump the **patch** (`0.2.0` → `0.2.1`).

Note: `CMakeLists.txt` `project(... VERSION ...)` may already be ahead of
`pyproject.toml`; the release should align both to the chosen version.

## Procedure

1. **Apply the edits.** Run the helper (edits files only, no git):

   ```pwsh
   pwsh .github/skills/prepare-release/scripts/prepare_release.ps1 -Version <X.Y.Z>
   ```

   It bumps `version` in [pyproject.toml](../../../pyproject.toml), aligns
   `project(slang_qrhi VERSION ...)` in [CMakeLists.txt](../../../CMakeLists.txt), and turns
   `## [Unreleased]` into a dated `## [X.Y.Z]` section with a fresh empty `## [Unreleased]`
   above it. Review the diff. Add any missing changelog bullets (features/fixes since the
   last release) in the author's voice.

2. **Rebuild** so the editable install reports the new version. Slang + Qt must be on the
   environment (see [repo memory / build notes]):

   ```pwsh
   $env:CMAKE_PREFIX_PATH="C:\dev\code\slang-qt\6.8.3\msvc2022_64"; $env:SLANG_ROOT="C:\dev\app\slang"
   .\.venv\Scripts\python.exe -m pip install --no-build-isolation -e .
   .\.venv\Scripts\python.exe -c "import importlib.metadata as m; print(m.version('miskeyed-workbench'))"
   ```

   The printed version must equal `X.Y.Z`.

3. **Test** (headless, CPU-only Slang reflection):

   ```pwsh
   $env:SLANG_ROOT="C:\dev\app\slang"; $env:QT_QPA_PLATFORM="offscreen"
   .\.venv\Scripts\python.exe -m pytest tests/ -q
   ```

   All tests must pass before committing.

4. **Commit** (the pre-commit hook runs; it's fine):

   ```pwsh
   git add pyproject.toml CMakeLists.txt CHANGELOG.md
   git commit -m "release: prepare X.Y.Z (version bump + changelog)"
   ```

5. **Hand off.** Tell the user it's ready to merge, and give the post-merge publish
   commands (do not run them):

   ```pwsh
   git tag vX.Y.Z; git push origin vX.Y.Z
   ```

## Gotchas

- Always use `.\.venv\Scripts\python.exe`; the system `python` is Python 2.7.
- `uv`/isolated builds fail — `shiboken6-generator` is only on Qt's index. Build with
  `--no-build-isolation` in the `.venv`.
- If a tag was already pushed but hasn't published, cancel the in-flight run first
  (`gh run cancel <id>`), then move the tag: `git tag -d vX.Y.Z; git push origin :refs/tags/vX.Y.Z; git tag vX.Y.Z; git push origin vX.Y.Z`.
- Confirm the published result at `https://pypi.org/pypi/miskeyed-workbench/X.Y.Z/json`.
