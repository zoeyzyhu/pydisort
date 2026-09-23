Contributing
============

Discuss substantial changes in a
`GitHub issue <https://github.com/zoeyzyhu/pydisort/issues>`_ first.
Small documentation corrections can go straight to a pull request.

Set up a working branch
------------------------

Fork the repository if you do not have push access, clone your fork, and
create a branch for the change:

.. code-block:: bash

   git switch -c your-name/describe-the-change

Follow :doc:`installation` to build from source in an isolated environment.
Then install the development tools:

.. code-block:: bash

   python -m pip install pytest pre-commit
   pre-commit install

Make and validate the change
----------------------------

Read neighboring tests and examples before changing behavior. Add or update
a regression test when practical. Keep generated outputs and local
environments out of commits, and stage only files relevant to the change.

.. code-block:: bash

   python -m pytest tests/ -v -rs
   pre-commit run --all-files

Pre-commit checks formatting and lint rules; it does not run the solver's
test suite. If a hook rewrites files, review its changes before staging them.
For the C/C++ checks, configure with ``-DBUILD_TESTS=ON``, build, and run:

.. code-block:: bash

   ctest --test-dir build --output-on-failure

See :doc:`testing` for the validation cases and skip conditions.

Documentation changes
~~~~~~~~~~~~~~~~~~~~~

Sphinx pages live in ``docs/source/``. The API reference reads descriptions
from ``python/pydisort.pyi`` in this checkout, without importing the compiled
package. Update that stub when changing the public API.

.. code-block:: bash

   python -m pip install -r docs/requirements.txt
   python -m unittest discover -s docs/_ext -p 'test_*.py'
   python -m sphinx -E -b html -W --keep-going docs/source docs/_build/html
   python -m sphinx -b doctest -W docs/source docs/_build/doctest

The HTML build needs only the documentation dependencies. Doctests also need
the built/installed solver and run the shared quickstart, type-hints and
batch-shape examples. Ordinary ``code-block`` examples are not executed automatically.
Preview ``docs/_build/html/index.html`` before submitting.

.. _separate-developer-checks:

Separate developer checks
~~~~~~~~~~~~~~~~~~~~~~~~~

The following checks are run separately from the CI/CTest solver suite.
Run them from the repository root when changing the relevant tooling:

.. code-block:: bash

   # Benchmark thread control and optional-dependency handling
   python -m pip install PythonicDISORT threadpoolctl
   python -m pytest benchmarks/tests/ -v -rs

   # Documentation API renderer (no compiled solver needed)
   python -m pip install -r docs/requirements.txt
   python -m unittest discover -s docs/_ext -p 'test_*.py'

   # Source-build installation guidance
   python -m pytest tests/test_setup_guidance.py -v -rs

The benchmark and renderer tests are not registered with CTest. The setup
guidance check runs with direct ``pytest tests/`` in a checkout, but its
CTest copy skips because the build tree does not contain the repository's
``setup.py``. Use the source-tree command above to exercise that check.
These checks are not added to GitHub Actions by the current workflow.

Submit a pull request
----------------------

.. code-block:: bash

   git diff --check
   git add path/to/changed-file
   git commit -m "Describe the change"
   git push -u origin your-name/describe-the-change

Open a PR against ``main``. Explain the problem, the changes, and the exact
validation commands and results, including any tests you could not run.
Push follow-up commits to the same branch to update the PR.

The repository uses squash merging to keep a linear main-branch history.
After the PR is merged and your worktree is clean, return to ``main`` and
update it with ``git pull --ff-only`` (use the upstream remote when working
from a fork). Keeping or deleting the old local branch is your choice.

Conventions and upstream code
------------------------------

Follow the style of neighboring code and the rules in
``.pre-commit-config.yaml``; do not reformat unrelated files.
Explain changes to the bundled ``cdisort213/`` separately from wrapper
changes, and preserve their upstream provenance. See :doc:`devops` for
build and release details.
