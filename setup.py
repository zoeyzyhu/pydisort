"""Setup configuration for Python packaging."""
# pylint: disable = deprecated-module, exec-used
import os
import sys
import platform
import subprocess
import shutil
import glob
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from torch.utils import cpp_extension

def parse_library_names(libdir):
    """Parse the library files."""
    # Get the library files
    library_files = []
    for root, _, files in os.walk(libdir):
        for file in files:
            if file.endswith('.a') or file.endswith('.so'):
                library_files.append(os.path.join(root, file))

    # Extract the library names from the library files
    library_names = []
    for library_file in library_files:
        file_name = os.path.basename(library_file)
        # remove lib and .so or .a
        library_name = file_name[3:].rsplit('.', 1)[0]
        library_names.append(library_name)
    return library_names

def check_requirements():
    """Check if the system requirements are met."""
    # Check the operating system
    os_name = platform.system()
    if os_name not in ['Darwin', 'Linux']:
        sys.stderr.write(
            "Unsupported operating system. Please use MacOS or Linux.\n")
        return False

    # Min python version is Python3.6
    if sys.version_info < (3, 6):
        sys.stderr.write("Python 3.6 or higher is required.\n")
        return False

    return True


class CMakeBuild(build_ext):
    """Define external build cmd for cmake."""

    def run(self):
        """Self-define run."""
        for ext in self.extensions:
            self.build_extension(ext)

        # Check if cmake is installed.
        if shutil.which('cmake'):
            sys.stderr.write("CMake is required to build this package.\n")
            sys.exit(-1)

    def build_extension(self, ext):
        """Build project"""
        #print("ext.name: ", ext.name)
        extdir = os.path.abspath(os.path.dirname(
            self.get_ext_fullpath(ext.name)))
        #print("extdir: ", extdir)

        cfg = 'Debug' if self.debug else 'Release'
        python_version = f'{sys.version_info.major}.{sys.version_info.minor}'

        # Adding CMake arguments set as environment variable
        # (needed e.g. to build for ARM OSx on conda-forge)
        build_args = []
        if "CMAKE_ARGS" in os.environ:
            build_args += [
                item for item in os.environ["CMAKE_ARGS"].split(" ") if item]

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
                f'-DPYTHON_VERSION={python_version}',
            ]
            cmake_configure_command.extend(build_args)
            #print(cmake_configure_command)
            subprocess.run(cmake_configure_command, check = True)
            subprocess.run(['cmake', '--build', _build_dir], check = True)
        except subprocess.CalledProcessError as e:
            sys.stderr.write(f"Error while building with CMake: {e.returncode}\n")
            sys.exit(-1)

# pylint: disable=too-few-public-methods

class CMakeExtension(Extension):
    """Define CMake extension."""

    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)
# pylint: enable=too-few-public-methods


# If the system does not meet requirement, exit.
if not check_requirements():
    sys.exit(1)

# Setup configuration
current_dir = Path().absolute()
setup(
    ext_modules=[cpp_extension.CUDAExtension(
        name = 'pydisort',
        sources = glob.glob('python/*.cpp') + glob.glob('src/**/*.cu', recursive=True),
        include_dirs = [f'{current_dir}',
                        f'{current_dir}/build',
                        f'{current_dir}/build/_deps/fmt-src/include'],
        library_dirs = [f'{current_dir}/build/lib'],
        libraries = parse_library_names(f'{current_dir}/build/lib'),
        extra_compile_args = {'nvcc': ['--extended-lambda']},
        )],
    cmdclass={'build_ext': cpp_extension.BuildExtension},
)

#setup(
#    ext_modules=[CMakeExtension('pydisort', 'python')],
#    cmdclass={'build_ext': CMakeBuild},
#)
