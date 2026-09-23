Python environments
===================

Use an isolated environment so that Python, pip, PyTorch and the compiled
extension agree. These commands are for bash/zsh on macOS or Linux.

Create and activate
-------------------

Choose a CPython version supported by the release you are installing
(see :doc:`installation`). From your project directory:

.. code-block:: bash

   python3 -m venv env
   source env/bin/activate
   python -m pip install --upgrade pip
   python -m pip install pydisort
   python -c "import sys, pydisort; print(sys.executable); print(pydisort.__version__)"

Activate the same environment in each new shell. Configure your IDE or
notebook kernel to use its Python interpreter too. Leave the environment with
``deactivate``; you do not need to delete it to switch projects.

For development
---------------

A source build needs the C++ library before the Python bindings. Follow
:doc:`installation` for that build sequence, then :doc:`contribute` for tests,
formatting and documentation checks.

Record the Python version, platform and package versions when reporting a
problem or reproducing a calculation:

.. code-block:: bash

   python --version
   python -m pip freeze

An environment isolates packages; it does not lock their versions.
Requirements without version pins may resolve differently on another day.

Troubleshooting environment mixing
----------------------------------

Check which interpreter and packages you are using:

.. code-block:: bash

   python -m pip --version
   python -c "import sys, torch; print(sys.executable); print(torch.__file__)"

A custom ``PYTHONPATH`` can redirect imports outside the environment. If
that is causing a conflict, temporarily clear it in the current shell and
retry:

.. code-block:: bash

   printenv PYTHONPATH
   unset PYTHONPATH

Do not remove unrelated shell configuration or globally installed packages.
If you need a clean comparison, create a second environment under a new name.

Conda
------

Conda environments are not currently tested in CI; this is not a blanket
claim that Anaconda's Python cannot work. Keep Python, pip, PyTorch and build
dependencies in the same activated environment. If a compiled-library or
import problem persists, reproduce it with the standard ``venv`` recipe
above and include both environments' details in the issue.
