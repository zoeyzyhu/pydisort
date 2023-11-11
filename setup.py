"""Setup configuration for Python packaging."""
# pylint: disable = deprecated-module, exec-used
import os
import sys
import platform
from distutils import spawn
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


def check_requirements():
    """Check if the system requirements are met."""
    # Check the operating system
    os_name = platform.system()
    if os_name not in ['Darwin', 'Linux']:
        sys.stderr.write(
            "Unsupported operating system. Please use MacOS or Linux.\n")
        return False

    # Min python version is Python3.8
    if sys.version_info < (3, 8):
        sys.stderr.write("Python 3.8 or higher is required.\n")
        return False

    return True


class CMakeBuild(build_ext):
    """Define external build cmd for cmake."""
    def run(self):
        """Self-define run."""
        for ext in self.extensions:
            self.build_extension(ext)

        # Check if cmake is installed.
        if spawn.find_executable('cmake') is None:
            sys.stderr.write("CMake is required to build this package.\n")
            sys.exit(-1)

    def build_extension(self, ext):
        """Build project"""
        print("ext.name: ", ext.name)
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        print("extdir: ", extdir)

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg, '--', '-j2']

        # Set up cmake configuration
        _source_dir = os.path.split(os.path.abspath(__file__))[0]
        _build_dir = os.path.join(_source_dir, 'build')
        try:
            cmake_configure_command = [
                'cmake',
                f'-H{_source_dir}',
                f'-B{_build_dir}',
                f'-DCMAKE_BUILD_TYPE={cfg}',
                f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}',
            ]
            spawn.spawn(cmake_configure_command)
            spawn.spawn(['cmake', '--build', _build_dir])
        except spawn.DistutilsExecError:
            sys.stderr.write("Error while building with CMake\n")
            sys.exit(-1)


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


# If the system does not meet requirement, exit.
if not check_requirements():
    sys.exit(1)

# Setup configuration
setup(
    ext_modules=[CMakeExtension('pydiosrt', 'python')],
    cmdclass=dict(build_ext = CMakeBuild),
    )
