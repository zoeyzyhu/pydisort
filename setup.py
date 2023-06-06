"""Setup configuration for Python packaging."""
# pylint: disable = deprecated-module, exec-used
import os
import sys
import platform
import distutils.command.build as _build
from distutils import spawn
from distutils.sysconfig import get_python_lib
from setuptools import setup


def check_requirements():
    """Check if the system requirements are met."""
    # Check the operating system
    os_name = platform.system()
    if os_name not in ['Darwin', 'Linux']:
        sys.stderr.write(
            "Unsupported operating system. Please use MacOS or Linux.\n")
        return False

    # Check the Python version
    if sys.version_info < (3, 6):
        sys.stderr.write("Python 3.6 or higher is required.\n")
        return False

    # For Mac, min is Python3.8
    if sys.version_info < (3, 8) and os_name == 'Darwin':
        sys.stderr.write("Python 3.8 or higher is required.\n")

    return True


def extend_build():
    """Define external build cmd for cmake."""
    class Build(_build.build):
        """Self-define build."""

        def run(self):
            """Self-define run."""
            # Check if cmake is installed.
            cwd = os.getcwd()
            if spawn.find_executable('cmake') is None:
                sys.stderr.write("CMake is required to build this package.\n")
                sys.exit(-1)

            # Set up cmake configuration
            _source_dir = os.path.split(os.path.abspath(__file__))[0]
            _build_dir = os.path.join(_source_dir, 'build_setup_py')
            _prefix = get_python_lib()
            try:
                cmake_configure_command = [
                    'cmake',
                    f'-H{_source_dir}',
                    f'-B{_build_dir}',
                    f'-DCMAKE_INSTALL_PREFIX={_prefix}',
                ]
                _generator = os.getenv('CMAKE_GENERATOR')
                if _generator is not None:
                    cmake_configure_command.append(f'-G{_generator}')
                spawn.spawn(cmake_configure_command)
                spawn.spawn(
                    ['cmake', '--build', _build_dir, '--target', 'install'])
                os.chdir(cwd)
            except spawn.DistutilsExecError:
                sys.stderr.write("Error while building with CMake\n")
                sys.exit(-1)
            _build.build.run(self)

    return Build


# If the system does not meet requirement, exit.
if not check_requirements():
    sys.exit(1)

# A few variables
_here = os.path.abspath(os.path.dirname(__file__))
version = {}
with open(os.path.join(_here, 'src/pydisort', 'version.py'), encoding='utf-8') as f:
    exec(f.read(), version)

# Setup configuration
setup(
    name='pydisort',
    version=version['__version__'],
    description='Modern Python interfece of DISORT.',
    author='Zoey Hu',
    author_email='zoey.zyhu@gmail.com',
    license='GPL',
    packages=['pydisort'],
    package_dir={'': 'src'},
    include_package_data=True,
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Science/Research',
        'Programming Language :: Python :: 3.6'
    ],
    python_requires=">=3.6",
    cmdclass={'build': extend_build()})
