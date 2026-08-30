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
artifacts: neither belongs in Git. Build in dependency order from a native,
installable Windows Workbench::

   python -m pip install -r src/docs/requirements.txt
   python ci/capture_workbench.py --all --output src/docs/images
   python ci/verify_doc_images.py src/docs/images
   sphinx-build -W --keep-going -b html src/docs build/documentation/docs/dev/local

``src/docs/images`` and ``docs`` are ignored output locations. Each scenario
configures size, tool, focus, bindings, inspector, document view, generated target,
timeline, and layout before asserting semantic state and grabbing the real window.
The Sphinx build therefore cannot pass against stale or missing captures.

Deployment channels
-------------------

CI computes a Pages-relative output from the Git ref rather than committing the
site. The stable channels are::

   main branch       -> docs/prod/main
   v0.3.0 tag        -> docs/prod/0.3.0
   release/0.3.0     -> docs/dev/release/0.3.0
   another branch    -> docs/dev/branch/<branch>

The release-branch path is a review artifact: it requires no release or Pages write
permission. Promotion/deployment can later publish the exact artifact after review.
``ci/docs_destination.py`` owns this mapping so local and CI staging agree.

Release work
------------

Keep ``CHANGELOG.md`` release-oriented, update the single package version in
``pyproject.toml``, run native/package checks, regenerate images, and build Sphinx
with warnings as errors. Deployment permissions are intentionally outside the docs
check.
