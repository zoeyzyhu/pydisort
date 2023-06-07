## How to set up Python virtual environment 📌

## Table of Contents

- [🔻 Prerequisites](#prerequisites)
- [🔻 Restarting this guide](#restarting-this-guide)
- [🔻 Install Python](#install-python)
  - [macOS](#macos)
  - [WSL or Linux](#wsl-or-linux)
- [🔻 Create a Python virtual environment](#create-a-python-virtual-environment)
- [🔻 Understanding virtual environments](#understanding-virtual-environments)
  - [Environment](#environment)
  - [Environment variables inside a Python program](#environment-variables-inside-a-python-program)
  - [Virtual environment](#virtual-environment)
  - [Why virtual environments?](#why-virtual-environments)
  - [Activate a virtual environment](#activate-a-virtual-environment)
  - [Replicate a virtual environment](#replicate-a-virtual-environment)
  - [Deactivate a virtual environment](#deactivate-a-virtual-environment)
- [🔻 Summary](#summary)

## <a id='prerequisites'>🔻 Prerequisites </a>

At this point, you should already have a folder for your project. Your folder location might be different.

```
$ pwd
/Users/zoeyzyhu/projects/pydisort
```

## <a id='restarting-this-guide'>🔻 Restarting this guide </a>

If you made a mistake with these Python instructions, here’s how to start over. First, close your shell and reopen it to ensure that environment variables are reset. Then, delete the virtual environment.

```
$ pwd
/Users/zoeyzyhu/projects/pydisort
$ rm -rf env
```

## <a id='install-python'>🔻 Install Python </a>

Install a recent version of Python.

### macOS

You might already have Python installed. Your version might be different.

```
$ python3 --version
Python 3.11.3
```

You can install a recent version of Python using the Homebrew package manager. Your version might be different.

```
$ brew install python3
$ python3 --version
Python 3.11.3
```

### WSL or Linux

```
$ sudo apt-get update
$ sudo apt-get install python3 python3-pip python3-venv
```

<div align="right">[ <a href="#table-of-contents">↑ Back to top ↑</a> ]</div>

## <a id='create-a-python-virtual-environment'>🔻 Create a Python virtual environment </a>

This section will help you install the Python tools and packages locally, which won’t affect Python tools and packages installed elsewhere on your computer.

After finishing this section, you’ll have a folder called `env/` that contains all the Python packages you need for this project.

> **Pitfall**: Do not use the version of Python provided by Anaconda.
>
> ```
> $ which python3
> /Users/zoeyzyhu/anaconda/bin/python3
> ```
>
> If you see `/anaconda/` in the path, then you’re using Anaconda. You’ll need to deactivate Anaconda before continuing.
>
> **Option 1 (recommended)**: Permanently deactivate Anaconda. After running this command, close your shell and reopen it.
>
> ```
> $ conda init --reverse
> ```
>
> Close your shell and open a new shell. Your path might be different.
>
> ```
> $ which python3
> /usr/local/bin/python3 # NOT anaconda
> ```
>
> **Option 2**: Temporarily deactivate Anaconda. You’ll have to do this every time you start a new shell. Your path might be different.
>
> ```
> $ conda deactivate
> $ which python3
> /usr/local/bin/python3 # NOT anaconda
> ```
>
> **Option 3**: Uninstall Anaconda completely ([docs](https://docs.anaconda.com/free/anaconda/install/uninstall/)).
>
> ```
> $ conda install anaconda-clean
> $ anaconda-clean --yes
> ```
>
> Close your shell and open a new shell. Your path might be different.
>
> ```
> $ which python3
> /usr/local/bin/python3 # NOT anaconda
> ```
>
> **Option 4**: Manually deactivate Anaconda. If none of the above options work, then this one will.
>
> Figure out which hidden shell startup file contains the Anaconda initialization code.
>
> ```
> $ pwd
> /Users/zoeyzyhu
> $ grep -s conda .profile .bashrc .bash_profile .zshrc .zlogin .cshrc .tshrc .login
> .bash_profile:# >>> conda initialize >>>
> .bash_profile:# !! Contents within this block are managed by 'conda init' !!
> ...
> ```
>
> In this case, the file to edit is `.bash_profile`. Yours might be different. Use any text editor. If you’re using VS Code, here’s a shortcut. Remember, your filename might be different.
>
> ```
> $ code .bash_profile
> ```
>
> Remove everything you find about Anaconda and save the file. In this case, we’ll delete a chunk that looks like this.
>
> ```
> # >>> conda initialize >>>
>
> # !! Contents within this block are managed by 'conda init' !!
>
> **conda_setup="$('/usr/local/anaconda3/bin/conda' 'shell.bash' 'hook' 2> /dev/null)"
> if [ $? -eq 0 ]; then
>     eval "$**conda_setup"
> else
> if [ -f "/usr/local/anaconda3/etc/profile.d/conda.sh" ]; then
> . "/usr/local/anaconda3/etc/profile.d/conda.sh"
> else
> export PATH="/usr/local/anaconda3/bin:$PATH"
> fi
> fi
> unset \_\_conda_setup
>
> # <<< conda initialize <<<
> ```
>
> Close your shell and open a new shell. Your path might be different.
>
> ```
> $ which python3
> /usr/local/bin/python3 # NOT anaconda
> ```

<div align="right">[ <a href="#table-of-contents">↑ Back to top ↑</a> ]</div>

> **Pitfall**: If the `PYTHONPATH` environment variable is set, it can cause problems.
>
> ```
> $ printenv PYTHONPATH # Output isn't blank, problem!
> /Users/zoeyzyhu/local/lib/python3.11/site-packages/
> ```
>
> **Option 1 (recommended)**: Permanently remove the environment variable. Variables are usually set in your shell initialization file. Check these files to see if they set the offending variable: `.profile`, `.bashrc`, `.bash_profile`, `.zshrc`, `.zprofile`, `.cshrc`, `.tcshrc`, `.login`. Delete or comment out any line that contains `PYTHONPATH`.
>
> ```
> $ pwd
> /Users/zoeyzyhu
> $ grep -s PYTHONPATH .profile .bashrc .bash_profile .zshrc .zlogin .cshrc .tshrc .login
> .bashrc: export PYTHONPATH=/Users/zoeyzyhu/local/lib/python3.9/site-packages/
>
> # Edit the file and remove the line.
>
> # Close your shell and open a new shell
>
> $ printenv PYTHONPATH # output should be blank
> ```
>
> **Option 2**: Temporarily unset the `PYTHONPATH` environment variable. You’ll have to do this every time you start a new shell.
>
> ```
> $ env --unset PYTHONPATH
> $ printenv PYTHONPATH # output should be blank
> ```
>
> Create a virtual environment in your project’s root directory. (More on [venv and the creation of virtual environments](https://docs.python.org/3/library/venv.html))

```
$ pwd
/Users/zoeyzyhu/projects/pydisort
$ python3 -m venv env
```

Activate virtual environment. You’ll need to do this every time you start a new shell.

```
$ source env/bin/activate
```

👏 We now have a complete local environment for Python. Everything lives in one directory. Environment variables point to this virtual environment.

```
$ echo $VIRTUAL_ENV
/Users/zoeyzyhu/projects/pydisort/env
```

We have a Python interpreter installed inside the virtual environment. which python tells you exactly which python executable file will be used when you type python. Because we’re in a virtual environment, there’s more than one option!

```
$ which python3 # Default python executable
/Users/zoeyzyhu/projects/pydisort/env/bin/python
$ which -a python # All python executables
/Users/zoeyzyhu/projects/pydisort/env/bin/python3
/usr/local/bin/python3
/usr/bin/python3
```

There’s a package manager for Python installed in the virtual environment. That will help us install Python packages later.

```
$ which pip
/Users/zoeyzyhu/projects/pydisort/env/bin/pip
$ pip --version
pip 23.1.2 from /Users/zoeyzyhu/projects/pydisort/env/lib/python3.11/site-packages (python 3.11) # Your version may be different
```

Python packages live in the virtual environment. We can see that Python’s own tools are already installed (`pip` and `setuptools`).

```
$ ls env/lib/python3.11/site-packages/ # Your version may be different
pip
setuptools
...
```

Upgrade the Python tools in your virtual environment

```
$ pip install --upgrade pip setuptools
```

<div align="right">[ <a href="#table-of-contents">↑ Back to top ↑</a> ]</div>

## <a id='understanding-virtual-environments'>🔻 Understanding Virtual Environments </a>

This section will give more detail about virtual environments and how they work. Simply put, a virtual environment is a bunch of files (located in `env/` in this tutorial) used by Python.

### Environment

An environment is a collection of environment variables that are inputs to your shell and your programs.

Print the names and values of all environment variables using the `env` command. You’ll see `key/value` pairs used by the shell and used by programs.

```
$ env
...
PWD=/Users/zoeyzyhu/projects/pydisort
HOME=/Users/zoeyzyhu
USER=zoeyzyhu
PATH=/usr/local/bin:/usr/bin:/bin
...
```

An important example of an environment variable is `PATH`, which tells your shell where to look for commands like `ls`, `cd`, `python` and so on. It’s a colon-separated list (`:`). You can print the value of one variable using the dollar sign `$` closely entailed by the variable.

```
$ echo $PATH
/usr/local/bin:/usr/bin:/bin
$ printenv PATH # Alternative
/usr/local/bin:/usr/bin:/bin
$ echo $PATH | tr ':' '\n'
/usr/local/bin
/usr/bin
/bin
```

Notice that each item in the list is a directory that contains executables, for example `/usr/local/bin` usually contains the `python3` executable on macOS with Homebrew (`/opt/homebrew/bin` on Apple Silicon M1).

```
$ ls /usr/local/bin
...
python3
...
```

### Environment variables inside a Python program

You can set any environment variable you want.

```
$ export MESSAGE="hello world"
$ echo $MESSAGE
hello world
```

Environment variables are accessible from programs, like this `test.py`.

```
"""test.py"""
import os
print(os.environ["MESSAGE"])
```

Set an environment variable and run the program.

```
$ export MESSAGE="hello world"
$ python3 test.py
hello world
```

This example shows that environment variables are simply another way to provide input to a running program.

### Virtual environment

A virtual environment is a self-contained directory that contains a Python installation and a number of additional Python packages.

As you saw earlier, the command to create a virtual environment creates a new directory, `env` in this example.

```
$ python3 -m venv env # you ran this earlier
$ ls env/
bin include lib pyvenv.cfg
```

The virtual environment contains a `bin/` directory with executables. It also contains a `lib/ `directory where Python third party packages live. Your versions might be different.

```
$ ls env/bin/
...
pip
python
...
$ ls env/lib/python3.11/site-packages/ # Your version may be different
**pycache** pip-23.1.2.dist-info setuptools-65.6.3.dist-info
easy_install.py pkg_resources pip setuptools
```

A pre-configured `pip` executable installs third party packages to `lib/`. Your versions of Python and jinja2 may be different.

```
$ ./env/bin/pip install tomli
Successfully installed tomli-2.0.1
$ ls env/lib/python3.11/site-packages/tomli/ # Your version may be different
**init**.py
...
```

A pre-configured `python` executable in `bin/` uses the third party packages in `lib/`.

```
$ ./env/bin/python
>>> import tomli
>>> tomli.**version**
>>> '2.0.1'
```

<div align="right">[ <a href="#table-of-contents">↑ Back to top ↑</a> ]</div>

### Why virtual environments?

Virtual environments are useful when you want to install different Python programs that have different third party package dependencies. For example, you might have a virtual environment for a `pydisort` project, and a different one for your machine learning project. The two projects have different third party packages and different versions of those packages.

### Activate a virtual environment

In the previous example, we used the virtual environment by calling its Python executable explicitly (e.g., `./env/bin/python`). As a convenience, we can temporarily make this version the default.

The `bin/activate` script adds `env/bin` to the `PATH` environment variable, making it the first place to look for commands. Notice that `/Users/zoeyzyhu/projects/pydisort/env/bin` is first in the list.

```
$ source env/bin/activate
$ echo $PATH
/Users/zoeyzyhu/projects/pydisort/env/bin:/usr/local/bin:/usr/bin:/bin
$ echo $PATH | tr ':' '\n'
/Users/zoeyzyhu/projects/pydisort/env/bin
/usr/local/bin
/usr/bin
/bin
```

Ask the shell where all the `python` executables live, then which one is the default.

```
$ which -a python
/Users/zoeyzyhu/projects/pydisort/env/bin/python
/usr/local/bin/python
/usr/bin/python
$ which python
/Users/zoeyzyhu/projects/pydisort/env/bin/python
```

Finally, the `activate` script sets a `$VIRTUAL_ENV` environment variable, which contains the path to the virtual environment directory.

```
$ echo $VIRTUAL_ENV
/Users/zoeyzyhu/projects/pydisort/env
```

### Replicate a virtual environment

In the previous section, we created a Python virtual environment, activated it, and upgraded the Python installer tools (`pip`, `setuptools`). We have not yet installed any new third party Python packages.

```
$ pwd
/Users/zoeyzyhu/projects/pydisort
$ echo $VIRTUAL_ENV
/Users/zoeyzyhu/projects/pydisort/env
$ pip list

Package Version

---

pip 22.3.1
setuptools 65.6.3
```

A `requirements.txt` file lists the exact third party Python packages and their versions needed to replicate another virtual environment. This is useful for ensuring that developers and production servers have identical packages with identical versions. It’s also useful for ensuring that students and the autograder have identical packages with identical versions.

See an example list of package dependencies provided in a `requirements.txt` file below.bei

```
$ cat requirements.txt
tomli==2.0.1
...
zipp==3.15.0
```

Install the package dependencies. Your output might be different.

```
$ pip install -r requirements.txt
...
Successfully installed tomli-2.0.1 ... zipp-3.15.0
$ pip list
Package Version

---

tomli 2.0.1
...
zipp 3.15.0
```

### Deactivate a virtual environment

The deactivate command simply modifies two environment variables, `PATH` and `VIRTUAL_ENV`. First, it unsets `VIRTUAL_ENV`.

```
$ deactivate
$ echo $VIRTUAL_ENV # Variable not set, output is blank
```

Finally, `deactivate` changes `PATH` to its previous value, before the virtual environment was activated.

```
$ echo $PATH | tr ':' '\n'
/usr/local/bin
/usr/bin
/bin
```

## <a id='summary'>🔻 Summary </a>

A Python virtual environment helps you manage third party packages. A pre-configured python executable in `./env/bin/` uses the third party packages in `./env/lib/` (the name of `env/` is your choice).

Activate the virtual environment each time you start a new shell.

```
$ pwd
/Users/zoeyzyhu/projects/pydisort
$ source env/bin/activate
```

The activate script changes the `PATH` environment variable, which temporarily changes the default python and pip executables.

```
$ which python
/Users/zoeyzyhu/projects/pydisort/env/bin/python
$ which pip
/Users/zoeyzyhu/projects/pydisort/env/bin/pip
```

<div align="right">[ <a href="#table-of-contents">↑ Back to top ↑</a> ]</div>
