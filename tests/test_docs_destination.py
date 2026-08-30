from pathlib import PurePosixPath

from ci.docs_destination import destination


def test_main_is_the_rolling_production_site():
    assert destination("refs/heads/main", "0.3.0") == PurePosixPath("docs/prod/main")


def test_release_tag_is_the_versioned_production_site():
    assert destination("refs/tags/v0.3.0", "0.3.0") == PurePosixPath("docs/prod/0.3.0")


def test_release_branch_is_a_review_site_before_release():
    assert destination("release/0.3.0", "0.3.0") == PurePosixPath("docs/dev/release/0.3.0")


def test_feature_branch_is_isolated_under_dev():
    assert destination("docs/teach-architecture", "0.3.0") == PurePosixPath(
        "docs/dev/branch/docs/teach-architecture"
    )
