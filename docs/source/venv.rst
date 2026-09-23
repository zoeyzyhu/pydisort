How to set up Python virtual environment
========================================

Prerequisites
-------------

At this point, you should already have a folder for your project. Your folder location might be different.

.. code-block:: bash

    $ pwd
    /Users/zoeyzyhu/projects/pydisort

Restarting this guide
---------------------

If you made a mistake with these Python instructions, here’s how to start over. First, close your shell and reopen it to ensure that environment variables are reset. Then, delete the virtual environment.

.. code-block:: bash

    $ pwd
    /Users/zoeyzyhu/projects/pydisort
    $ rm -rf env

Install Python
--------------

Install a recent version of Python.

macOS
~~~~~

You might already have Python installed. Your version might be different.

.. code-block:: bash

    $ python3 --version
    Python 3.11.3

You can install a recent version of Python using the Homebrew package manager. Your version might be different.

.. code-block:: bash

    $ brew install python3
    $ python3 --version
    Python 3.11.3

WSL or Linux
~~~~~~~~~~~~

.. code-block:: bash

    $ sudo apt-get update
    $ sudo apt-get install python3 python3-pip python3-venv


Create a Python virtual environment
-----------------------------------

This section will help you install the Python tools and packages locally, which won’t affect Python tools and packages installed elsewhere on your computer.

After finishing this section, you’ll have a folder called `env/` that contains all the Python packages you need for this project.

.. note::

    These instructions use a standard Python virtual environment. Conda
    environments are not currently tested in CI. If using conda, keep Python,
    pip, PyTorch, and build dependencies in the same activated environment.


.. warning::

    **Pitfall**: If the ``PYTHONPATH`` environment variable is set, it can cause problems.

    .. code-block:: bash

        $ printenv PYTHONPATH # Output isn't blank, problem!
        /Users/zoeyzyhu/local/lib/python3.11/site-packages/

    **Option 1 (recommended)**: Permanently remove the environment variable. Variables are usually set in your shell initialization file. Check these files to see if they set the offending variable: `.profile`, `.bashrc`, `.bash_profile`, `.zshrc`, `.zprofile`, `.cshrc`, `.tcshrc`, `.login`. Delete or comment out any line that contains ``PYTHONPATH``.

    .. code-block:: bash

        $ pwd
        /Users/zoeyzyhu
        $ grep -s PYTHONPATH .profile .bashrc .bash_profile .zshrc .zlogin .cshrc .tshrc .login
        .bashrc: export PYTHONPATH=/Users/zoeyzyhu/local/lib/python3.11/site-packages/

        # Edit the file and remove the line.

        # Close your shell and open a new shell

        $ printenv PYTHONPATH # output should be blank


    **Option 2**: Temporarily unset the ``PYTHONPATH`` environment variable. You’ll have to do this every time you start a new shell.

    .. code-block:: bash

        $ env --unset PYTHONPATH
        $ printenv PYTHONPATH # output should be blank


    Create a virtual environment in your project’s root directory. (More on [venv and the creation of virtual environments](https://docs.python.org/3/library/venv.html))

.. code-block:: bash

    $ pwd
    /Users/zoeyzyhu/projects/pydisort
    $ python3 -m venv env

Activate virtual environment. You’ll need to do this every time you start a new shell.

.. code-block:: bash

    $ source env/bin/activate

We now have a complete local environment for Python. Everything lives in one directory. Environment variables point to this virtual environment.

.. code-block:: bash

    $ echo $VIRTUAL_ENV
    /Users/zoeyzyhu/projects/pydisort/env

We have a Python interpreter installed inside the virtual environment. which python tells you exactly which python executable file will be used when you type python. Because we’re in a virtual environment, there’s more than one option!

.. code-block:: bash

    $ which python3 # Default python executable
    /Users/zoeyzyhu/projects/pydisort/env/bin/python
    $ which -a python # All python executables
    /Users/zoeyzyhu/projects/pydisort/env/bin/python3
    /usr/local/bin/python3
    /usr/bin/python3

There’s a package manager for Python installed in the virtual environment. That will help us install Python packages later.

.. code-block:: bash

    $ which pip
    /Users/zoeyzyhu/projects/pydisort/env/bin/pip
    $ pip --version
    pip 23.1.2 from /Users/zoeyzyhu/projects/pydisort/env/lib/python3.11/site-packages (python 3.11) # Your version may be different

Python packages live in the virtual environment. We can see that Python’s own tools are already installed (`pip` and `setuptools`).

.. code-block:: bash

    $ ls env/lib/python3.11/site-packages/ # Your version may be different
    pip
    setuptools
    ...

Upgrade the Python tools in your virtual environment

.. code-block:: bash

    $ pip install --upgrade pip setuptools


Understanding Virtual Environments
----------------------------------

This section will give more detail about virtual environments and how they work. Simply put, a virtual environment is a bunch of files (located in `env/` in this tutorial) used by Python.

Environment
~~~~~~~~~~~

An environment is a collection of environment variables that are inputs to your shell and your programs.

Print the names and values of all environment variables using the `env` command. You’ll see `key/value` pairs used by the shell and used by programs.

.. code-block:: bash

    $ env
    ...
    PWD=/Users/zoeyzyhu/projects/pydisort
    HOME=/Users/zoeyzyhu
    USER=zoeyzyhu
    PATH=/usr/local/bin:/usr/bin:/bin
    ...

An important example of an environment variable is `PATH`, which tells your shell where to look for commands like `ls`, `cd`, `python` and so on. It’s a colon-separated list (`:`). You can print the value of one variable using the dollar sign `$` closely entailed by the variable.

.. code-block:: bash

    $ echo $PATH
    /usr/local/bin:/usr/bin:/bin
    $ printenv PATH # Alternative
    /usr/local/bin:/usr/bin:/bin
    $ echo $PATH | tr ':' '\n'
    /usr/local/bin
    /usr/bin
    /bin

Notice that each item in the list is a directory that contains executables, for example `/usr/local/bin` usually contains the `python3` executable on macOS with Homebrew (`/opt/homebrew/bin` on Apple Silicon M1).

.. code-block:: bash

    $ ls /usr/local/bin
    ...
    python3
    ...

Environment variables inside a Python program
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can set any environment variable you want.

.. code-block:: bash

    $ export MESSAGE="hello world"
    $ echo $MESSAGE
    hello world

Environment variables are accessible from programs, like this `test.py`.

.. code-block:: python

    """test.py"""
    import os
    print(os.environ["MESSAGE"])

Set an environment variable and run the program.

.. code-block:: bash

    $ export MESSAGE="hello world"
    $ python3 test.py
    hello world

This example shows that environment variables are simply another way to provide input to a running program.

Virtual environment
~~~~~~~~~~~~~~~~~~~

A virtual environment is a self-contained directory that contains a Python installation and a number of additional Python packages.

As you saw earlier, the command to create a virtual environment creates a new directory, `env` in this example.

.. code-block:: bash

    $ python3 -m venv env # you ran this earlier
    $ ls env/
    bin include lib pyvenv.cfg

The virtual environment contains a `bin/` directory with executables. It also contains a `lib/` directory where Python third party packages live. Your versions might be different.

.. code-block:: bash

    $ ls env/bin/
    ...
    pip
    python
    ...
    $ ls env/lib/python3.11/site-packages/ # Your version may be different
    **pycache** pip-23.1.2.dist-info setuptools-65.6.3.dist-info
    easy_install.py pkg_resources pip setuptools

A pre-configured `pip` executable installs third party packages to `lib/`. Your versions of Python and jinja2 may be different.

.. code-block:: bash

    $ ./env/bin/pip install tomli
    Successfully installed tomli-2.0.1
    $ ls env/lib/python3.11/site-packages/tomli/ # Your version may be different
    **init**.py
    ...

A pre-configured `python` executable in `bin/` uses the third party packages in `lib/`.

.. code-block:: bash

    $ ./env/bin/python
    >>> import tomli
    >>> tomli.**version**
    >>> '2.0.1'


Why virtual environments?
~~~~~~~~~~~~~~~~~~~~~~~~~

Virtual environments are useful when you want to install different Python programs that have different third party package dependencies. For example, you might have a virtual environment for a `pydisort` project, and a different one for your machine learning project. The two projects have different third party packages and different versions of those packages.

Activate a virtual environment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In the previous example, we used the virtual environment by calling its Python executable explicitly (e.g., `./env/bin/python`). As a convenience, we can temporarily make this version the default.

The `bin/activate` script adds `env/bin` to the `PATH` environment variable, making it the first place to look for commands. Notice that `/Users/zoeyzyhu/projects/pydisort/env/bin` is first in the list.

.. code-block:: bash

    $ source env/bin/activate
    $ echo $PATH
    /Users/zoeyzyhu/projects/pydisort/env/bin:/usr/local/bin:/usr/bin:/bin
    $ echo $PATH | tr ':' '\n'
    /Users/zoeyzyhu/projects/pydisort/env/bin
    /usr/local/bin
    /usr/bin
    /bin

Ask the shell where all the `python` executables live, then which one is the default.

.. code-block:: bash

    $ which -a python
    /Users/zoeyzyhu/projects/pydisort/env/bin/python
    /usr/local/bin/python
    /usr/bin/python
    $ which python
    /Users/zoeyzyhu/projects/pydisort/env/bin/python

Finally, the `activate` script sets a `$VIRTUAL_ENV` environment variable, which contains the path to the virtual environment directory.

.. code-block:: bash

    $ echo $VIRTUAL_ENV
    /Users/zoeyzyhu/projects/pydisort/env

Replicate a virtual environment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In the previous section, we created a Python virtual environment, activated it, and upgraded the Python installer tools (`pip`, `setuptools`). We have not yet installed any new third party Python packages.

.. code-block:: bash

    $ pwd
    /Users/zoeyzyhu/projects/pydisort
    $ echo $VIRTUAL_ENV
    /Users/zoeyzyhu/projects/pydisort/env
    $ pip list

    Package Version

    ---

    pip 22.3.1
    setuptools 65.6.3

Third party packages are declared in ``pyproject.toml`` rather than in a
``requirements.txt`` file. ``dependencies`` lists what pydisort needs at run
time and ``build-system.requires`` lists what is needed to compile it, so one
``pip install`` reproduces the same packages at the same versions for every
developer.

.. code-block:: bash

    $ sed -n '/^dependencies/,/]/p' pyproject.toml
    dependencies = [
      "numpy",
      "torch==2.10.0",
    ]

Installing the project pulls those in. Use an editable install while working on
the repository, and disable build isolation so the build can see the ``torch``
already in this environment. Your versions may be different.

.. code-block:: bash

    $ pip install 'torch==2.10.0'
    $ pip install -e . --no-build-isolation
    ...
    $ pip list

    Package  Version

    ---

    numpy    2.5.2
    pydisort 1.8.5
    torch    2.10.0

The development tools are installed separately, since they are not needed to
use the package.

.. code-block:: bash

    $ pip install pre-commit pytest

.. note::

    ``pip install -e .`` links against the libraries CMake wrote into
    ``build/lib``, so configure and build with CMake first. :doc:`devops`
    explains how the two steps fit together.

Deactivate a virtual environment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The deactivate command simply modifies two environment variables, `PATH` and `VIRTUAL_ENV`. First, it unsets `VIRTUAL_ENV`.

.. code-block:: bash

    $ deactivate
    $ echo $VIRTUAL_ENV # Variable not set, output is blank

Finally, `deactivate` changes `PATH` to its previous value, before the virtual environment was activated.

.. code-block:: bash

    $ echo $PATH | tr ':' '\n'
    /usr/local/bin
    /usr/bin
    /bin

Summary
-------

A Python virtual environment helps you manage third party packages. A pre-configured python executable in `./env/bin/` uses the third party packages in `./env/lib/` (the name of `env/` is your choice).

Activate the virtual environment each time you start a new shell.

.. code-block:: bash

    $ pwd
    /Users/zoeyzyhu/projects/pydisort
    $ source env/bin/activate

The activate script changes the `PATH` environment variable, which temporarily changes the default python and pip executables.

.. code-block:: bash

    $ which python
    /Users/zoeyzyhu/projects/pydisort/env/bin/python
    $ which pip
    /Users/zoeyzyhu/projects/pydisort/env/bin/pip
