Developer's guide to this repo
==============================

pre-commit hooks
~~~~~~~~~~~~~~~~

This repo uses `pre-commit` hooks to ensure that code is formatted correctly and that tests pass before committing.
This `pre-commit` hook is defined in the `.pre-commit-config.yaml` file in the root directory of this repo.
To install the `pre-commit` hooks, run `pre-commit install` in the root directory.
This will install the `pre-commit` hooks in the local `.git` directory.
The `pre-commit` hooks will run automatically when you try to commit code.
If the `pre-commit` hooks fail, the commit will be aborted.
To run the `pre-commit` hooks manually, run `pre-commit run --all-files` in the root directory of this repo.

CI/CD
~~~~~

Starting from v0.5, This repo enables continuous deployment (CD). The main resource we consult for configuring CD
is the `cantera` repo: https://github.com/Cantera/cantera.

The `cantera` repo has three workflows:

- `main.yml`
- `packaging.yml`
- `post-merge-tests.yml`

In the `cantera` repo, the `main.yml` workflow is triggered by

1. push to the `main` branch.
2. pull request to the `main` branch.
3. release creation.

The following code snippet explains rule:

.. code-block:: yaml

    on:
      push:
        # Build on tags that look like releases
        tags:
          - v*
        # Build when main or testing is pushed to
        branches:
          - main
          - testing
      pull_request:
        # Build when a pull request targets main
        branches:
          - main

The `main.yml` workflow
~~~~~~~~~~~~~~~~~~~~~~~

The `main.yml` workflow builds multiple `cantera` libraries (`.so` files) and Python wheels using the CI-based matrix runner.
For example, the following code snippet shows how to build the library for different python versions:

.. code-block:: yaml

    ubuntu-multiple-pythons:
      name: ${{ matrix.os }} with Python ${{ matrix.python-version }}
      runs-on: ${{ matrix.os }}
      timeout-minutes: 60
      strategy:
        matrix:
          python-version: ["3.8", "3.10", "3.11"]
          os: ["ubuntu-20.04", "ubuntu-22.04"]
        fail-fast: false

Once built, these libraries and wheels are uploaded for future use (explained later).
We can use the following code snippet to upload the wheels:

.. code-block:: yaml

    - name: Save the wheel file to install Cantera
      uses: actions/upload-artifact@v3
      with:
        path: build/python/dist/Cantera*.whl
        retention-days: 2
        name: cantera-wheel-${{ matrix.python-version }}-${{ matrix.os }}
        if-no-files-found: error

Similarly, multiple python versions are built with clang on MacOS.

During the test phase, these libraries and wheels are downloaded and installed for testing.
For example, the following code downloads libraries and wheels for testing:

.. code-block:: yaml

      - name: Download the wheel artifact
        uses: actions/download-artifact@v3
        with:
          name: cantera-wheel-${{ matrix.python-version }}-${{ matrix.os }}
          path: dist
      - name: Download the Cantera shared library (.so)
        uses: actions/download-artifact@v3
        with:
          name: libcantera_shared-${{ matrix.os }}.so
          path: build/lib


The `packaging.yml` workflow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The packaging.yml workflow builds the python/conda packages and upload them to pypi automatically.
Specifically, we can use the `gh` command to trigger a manual run of github actions.
You can install `gh` on a mac using `brew install gh`. With `gh`, a dispatch workflow is defined as:

.. code-block:: yaml

    workflow_dispatch:  # allow manual triggering of this workflow
      inputs:
        outgoing_ref:
          description: "The ref to be built. Can be a tag, commit hash, or branch name"
          required: true
          default: "main"
        upload_to_pypi:
          description: "Try to upload wheels and sdist to PyPI after building"
          required: false
          default: "false"
        upload_to_anaconda:
          description: "Try to upload package to Anaconda after building"
          required: false
          default: "false"

The action steps that build and upload the pypi packages are here:

.. code-block:: yaml

      - name: Trigger PyPI/Wheel builds
        run: >
          gh workflow run -R cantera/pypi-packages
          python-package.yml
          -f incoming_ref=${{ env.REF }}
          -f upload=${{ env.UPLOAD_TO_PYPI }}
        env:
          GITHUB_TOKEN: ${{ secrets.PYPI_PACKAGE_PAT }}

Note that, in the code above, `cantera/pypi-packages` is **another repository** that contains the workflow for building
a python package and uploading it to pypi (see https://github.com/Cantera/pypi-packages).

Inspecting the `pypi-packages` repo, we can find the workflow file `python-package.yml` that builds the python package
The most important part is the `linux-wheel` step. Here is a simple illustration

.. code-block:: yaml

  linux-wheel:
    name: Build ${{ matrix.libc }}linux_${{ matrix.arch }} for py${{ matrix.py }}
    runs-on: ubuntu-20.04
    needs: ["sdist", "post-pending-status"]
    outputs:
      job-status: ${{ job.status }}
    strategy:
      matrix:
        py: ["38", "39", "310", "311"]
        arch: ["x86_64", "i686"]
        libc: ["many", "musl"]
        include:
          - py: "311"
            arch: "aarch64"
            libc: "many"
          - py: "311"
            arch: "ppc64le"
            libc: "many"
          ...

This builds a matrix combining different Python versions, architectures, and libc.
The building steps include downloading the pre-built libraries (in this case, sdist):

.. code-block:: yaml

    steps:
      - name: Download pre-built sdist
        uses: actions/download-artifact@v3
        with:
          name: sdist

and building the wheels using `cibuildwheel`:

.. code-block:: yaml

      - name: Set up QEMU
        uses: docker/setup-qemu-action@v2
        with:
          platforms: all
      - name: Build wheels
        uses: pypa/cibuildwheel@v2.12.3

and archiving (uploading) them:

.. code-block:: yaml

      - name: Archive the built wheels
        uses: actions/upload-artifact@v3
        with:
          path: ./wheelhouse/*.whl
          name: wheels

The major difference between `pydisort` and `cantera` is that `pydisort` is built with `pybind11` and `cmake`,
while `cantera` is built with `cython` and `scons`.

Build System - cmake
~~~~~~~~~~~~~~~~~~~~

Placeholder.

Reference articles
~~~~~~~~~~~~~~~~~~

- https://www.the-analytics.club/python-code-formatting-git-pre-commit-hook
