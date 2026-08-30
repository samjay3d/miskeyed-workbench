Contributing and documentation
==============================

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
site as a **Documentation Preview Artifact** and have read-only repository
permission; an artifact is not a public deployment.

Publication and versioning
--------------------------

The canonical public entry point is ``|documentation_url|``. Generated output is committed
only to the disposable ``docs`` deployment branch. A release publishes an immutable
version directory such as ``/0.3.0/`` and updates the root redirect to that version.
Existing version directories are preserved. Trusted pushes to ``main`` or
``release/**`` use the same built artifact to replace the mutable ``/dev/`` site;
arbitrary pull requests never publish publicly.

The generated branch layout is::

   index.html          stable redirect to the current release
   .nojekyll
   dev/                mutable trusted development site
   0.3.0/              immutable 0.3.0 release site
   0.4.0/              future immutable release, without deleting 0.3.0
   previews/           reserved for a future explicit trusted-preview policy

Sphinx names copied image assets ``_images/`` inside each complete site, alongside
``_static/``, architecture, Slang, rendering, and tool pages.

One-time maintainer setup is required **before publishing the package**. Ensure a
``docs`` branch exists, then open **Settings → Pages**, choose **Deploy from a
branch**, select the ``docs`` branch and the ``/ (root)`` folder. The workflow can
create the branch on its first attempt, but Pages must then be configured and the
release rerun. Ordinary pull-request jobs never receive content-write permission.
Repository branch rules must allow ``GITHUB_TOKEN`` to update the generated branch.

The **documentation** environment is used by both trusted development and release
publication jobs. Pushes to trusted branches publish
``|development_url|`` and place that clickable URL
in the Actions job summary.

The 0.3.0 gate runs in this order: detect the release; build distributions; install
the Windows 3.11 release wheel; capture and verify images; build Sphinx; publish and
verify the ``docs`` branch and retry the public URL while Pages propagates; publish
TestPyPI; publish PyPI; then create the tag and GitHub Release with a link to
``/0.3.0/``. A documentation failure therefore blocks
the package release.

Release work
------------

Keep ``CHANGELOG.md`` release-oriented, update the single package version in
``pyproject.toml``, run native/package checks, regenerate images, and build Sphinx
with warnings as errors. Deployment permissions are intentionally outside the docs
check.
