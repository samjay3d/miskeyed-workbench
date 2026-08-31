Contributing and documentation
==============================

Start with the repository ``AGENTS.md`` contract. Architectural work follows the
reusable ``.github/skills/architecture-review/SKILL.md`` audit; documentation scope is
selected with ``.github/skills/docs/SKILL.md``. These checklists point to canonical
code and documentation rather than duplicating the architecture.

Build and test
--------------

Use the environment-provided Qt and Slang SDKs. The normal native checks are CTest
and the Python tests configured by CI. Do not introduce compiler subprocesses or a
Python render-core mirror.

Teaching documentation
----------------------

Authored text lives in ``src/docs``. Screenshots and Sphinx HTML are generated
artifacts: neither belongs in Git. A single command performs the required native
capture, image-manifest validation, and warning-as-error Sphinx build::

   python -m pip install -r src/docs/requirements.txt
   python -m ci.build_docs --output build/documentation/site

The helper writes temporary captures to ignored ``src/docs/images`` before Sphinx
copies the same verified files into the site. Pull requests upload that complete
site as a **Documentation Preview Artifact**. The native build remains read-only.
A same-repository PR may pass that already-built artifact to the dedicated publisher
and update ``/dev/``; a fork PR can never enter the write-enabled job.

Publication and versioning
--------------------------

The canonical public entry point is |documentation_url|. Generated output is
committed only to the disposable ``docs`` deployment branch. A release publishes an immutable
version directory such as ``/0.3.0/`` and updates the root redirect to that version.
Existing version directories are preserved. Trusted pushes to ``main`` or
``release/**`` and same-repository PRs use the same built artifact to replace the
mutable ``/dev/`` site. Fork PRs remain artifact-only.

The generated branch layout is::

   index.html          stable redirect to the current release
   .nojekyll
   dev/                mutable trusted development site
   X.Y.Z/              immutable release site
   next-version/       future immutable release, without deleting X.Y.Z
   previews/           reserved for a future explicit trusted-preview policy

Sphinx names copied image assets ``_images/`` inside each complete site, alongside
``_static/``, architecture, Slang, rendering, and tool pages.

One-time maintainer setup is required **before publishing the package**. Open
**Settings → Pages** and choose **GitHub Actions** as the deployment source. The
publisher keeps the ``docs`` branch as generated history, then uploads that complete
tree with ``actions/upload-pages-artifact`` and deploys it with
``actions/deploy-pages``. It does not rely on the ``GITHUB_TOKEN`` branch push to
trigger Pages. Native build/test jobs and fork PRs never receive publication
permission. Only the dedicated publisher job receives ``contents: write``,
``pages: write``, and ``id-token: write``. Branch rules must allow ``GITHUB_TOKEN`` to
update the generated branch.

GitHub creates the **github-pages** environment automatically when the Actions source
is configured; maintainers do not need to pre-create it. Do not add required reviewers,
because that would introduce a manual release gate. If deployment branch/tag rules are
enabled, allow ``main`` and any intentionally pushed ``v*`` tags used by the manual
re-release path.

The environment is shared by development and release publication jobs. Only a trusted
``main`` push replaces |development_url| and performs a development deployment;
pull requests retain their reviewable documentation artifact without publishing the
repository's live site. The mutable ``/dev/`` gate verifies the build, screenshots,
generated branch contents, Pages artifact, and explicit deployment. Immutable release
publication additionally retains the stricter public-URL check before TestPyPI and PyPI.

The release gate runs in this order: detect the release; build distributions; install
the Windows 3.11 release wheel; capture and verify images; build Sphinx; update and
verify the ``docs`` history branch; upload and deploy the complete Pages site; retry
the public URL while Pages propagates; publish TestPyPI; publish PyPI; then create the
tag and GitHub Release with a link to
the immutable ``/<version>/`` documentation. A documentation failure therefore blocks
the package release.

Release work
------------

Release distributions cross an explicit candidate boundary. The reusable
``build-distributions.yml`` workflow builds and validates the native matrix, then seals
the uniquely named wheel transports, sdist, validation identities, source SHA, version,
and SHA-256 digests into the ``release-candidate`` artifact. Candidate artifacts are
retained for 60 days. Publication consumes that envelope through
``ci/assemble_release.py``; it does not rediscover or flatten matrix payloads.

If TestPyPI, PyPI, or tag creation fails after the candidate was sealed, dispatch the
**release** workflow from the Actions page. Supply the failed source run ID and expected
version. The workflow accepts only a release run whose candidate-sealing job succeeded
from ``main`` or ``release/*``, verifies that the source commit and version exist, then
checks the manifest, all digests, tar safety, wheel identities, and wheel CRCs before it
promotes anything. The manifest must contain all 12 Windows, Linux, macOS arm64, and
macOS x64 wheels for Python 3.11--3.13. Resume invokes no compiler, Qt SDK, Slang SDK,
or native matrix job and retains the existing ``release.yml`` trusted-publisher identity.
TestPyPI and PyPI use skip-existing behavior; an existing tag/release is accepted only
when it identifies the candidate source commit.

GitHub's **Re-run failed jobs** and a dispatched release resume are intentionally
different. A rerun uses the original run's source SHA and workflow definition. Resume
uses the current, corrected publication workflow with the old immutable candidate. Use
resume when publication orchestration itself has been fixed. An expired artifact, a
different manifest/version/SHA, a failed source run, or a disallowed source branch must
be rebuilt rather than bypassed.

Product/package inputs (``cpp/**``, ``bindings/**``, ``shaders/**``,
``python/miskeyed/**``, CMake/package metadata, installed contracts, and the candidate
build workflow) invalidate the native candidate. Publication helpers/workflows and
documentation orchestration run their focused Python or documentation checks without
starting the release matrix. Documentation changes still rebuild and verify the site.

Read release health by evidence level: **Package** is a produced wheel, **Installed**
is a fresh-environment install/import, **Contracts** is the installed-package or native
contract suite, and **Runtime** is an actual QRhi draw through
``miskeyed-workbench --rhi <backend> --rhi-smoke-test``. A release support claim must
have a passing lane at the claimed level. Checkout-dependent architecture tests stay
in the source-tree suite; the installed-wheel suite must exercise only public package
behavior and packaged resources. Runner and ICD setup belongs in the workflow.

Validation follows a three-level confidence ladder. Ordinary development PRs run the
focused Python 3.11 source/native checks. A push to, or PR targeting, any ``release/*``
branch additionally runs **Release Stabilization** across Windows, Linux, and both macOS
architectures on
Python 3.11 and 3.13; Python 3.11 owns native contracts and runtime smoke, while 3.13
proves the newest supported wheel and installed contracts. Any PR targeting ``main``
and every push to ``main`` runs **Main Integration**, the exhaustive Python 3.11--3.13
distribution matrix. Stabilization is read-only and can never publish. Release detection
after the main gate alone decides whether immutable docs, TestPyPI, PyPI, a tag, and a
GitHub Release follow.

Branch protection requires the stable aggregate ``CI`` check. Confidence labels remain on
the component jobs so changing a destination label cannot leave the required context
waiting for a name that no workflow emits.

Keep ``CHANGELOG.md`` release-oriented, update the single package version in
``pyproject.toml``, run native/package checks, regenerate images, and build Sphinx
with warnings as errors. Deployment permissions are intentionally outside the docs
check.

Ordinary post-release work belongs under ``[Unreleased]``. Work explicitly included in
an active release belongs in that release record. Do not encode one version's state as
a permanent contributor rule; the ``prepare-release`` skill handles the mechanical
transition when a release is cut.
