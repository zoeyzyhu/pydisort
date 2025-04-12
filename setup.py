"""Setup configuration for Python packaging."""
# pylint: disable = deprecated-module, exec-used
import os
import sys
import sysconfig
import platform
import glob
import torch
from pathlib import Path
from setuptools import setup
from torch.utils import cpp_extension

# Determine the torch library directory.
torch_lib_dir = os.path.join(os.path.dirname(torch.__file__), "lib")
torch_include_dir = torch.utils.cpp_extension.include_paths()
site_packages_dir = sysconfig.get_path("purelib")


def parse_library_names(libdir):
    """Parse the library files."""
    # Get the library files
    library_files = []
    for root, _, files in os.walk(libdir):
        for file in files:
            if file.endswith(".a") or file.endswith(".so"):
                library_files.append(os.path.join(root, file))

    # Extract the library names from the library files
    library_names = []
    for library_file in library_files:
        file_name = os.path.basename(library_file)
        # remove lib and .so or .a
        library_name = file_name[3:].rsplit(".", 1)[0]
        library_names.append(library_name)

    return library_names


def check_requirements():
    """Check if the system requirements are met."""
    # Check the operating system
    os_name = platform.system()
    if os_name not in ["Darwin", "Linux"]:
        sys.stderr.write(
            "Unsupported operating system. Please use MacOS or Linux.\n"
        )
        return False

    # Min python version is Python3.8
    if sys.version_info < (3, 8):
        sys.stderr.write("Python 3.8 or higher is required.\n")
        return False

    return True


# If the system does not meet requirements, exit.
if not check_requirements():
    sys.exit(1)

# Setup configuration
current_dir = os.getenv("WORKSPACE")
if not current_dir:
    current_dir = Path().absolute()

# Build a list of library directories.
# We add both our build directory and the torch library directory.
lib_dirs = [
    f"{current_dir}/build/lib",
    torch_lib_dir,
    site_packages_dir,
] + torch_include_dir

# For rpath settings, we want the runtime linker to search the torch library
# directory. (On macOS, extra_link_args will be used to embed this path
# into the binary.)
extra_link_args = []
if platform.system() == "Darwin":
    extra_link_args.extend(
        [
            f"-Wl,-rpath,{torch_lib_dir}",
            "-Wl,-rpath,@loader_path/.dylibs",
            "-Wl,-rpath,@executable_path/.dylibs",
        ]
    )
else:
    extra_link_args.extend(
        [f"-Wl,-rpath,{torch_lib_dir}", "-Wl,-rpath,$ORIGIN/.libs"]
    )

if torch.cuda.is_available():
    setup(
        name="pydisort",
        package_dir={"pydisort": "python"},
        packages=["pydisort"],
        ext_modules=[
            cpp_extension.CUDAExtension(
                name="pydisort.pydisort",
                sources=glob.glob("python/csrc/*.cpp")
                + glob.glob("src/**/*.cu", recursive=True),
                include_dirs=[
                    f"{current_dir}",
                    f"{current_dir}/build",
                    f"{current_dir}/build/_deps/fmt-src/include",
                ]
                + torch_include_dir,
                library_dirs=lib_dirs,
                libraries=[
                    "torch_global_deps",
                ]
                + parse_library_names(f"{current_dir}/build/lib"),
                extra_compile_args={"nvcc": ["--extended-lambda"]},
                extra_link_args=extra_link_args,
            )
        ],
        cmdclass={
            "build_ext": cpp_extension.BuildExtension,
        },
    )
else:
    setup(
        name="pydisort",
        package_dir={"pydisort": "python"},
        packages=["pydisort"],
        ext_modules=[
            cpp_extension.CppExtension(
                name="pydisort.pydisort",
                sources=glob.glob("python/csrc/*.cpp"),
                include_dirs=[
                    f"{current_dir}",
                    f"{current_dir}/build",
                    f"{current_dir}/build/_deps/fmt-src/include",
                ]
                + torch_include_dir,
                library_dirs=lib_dirs,
                libraries=[
                    "torch_global_deps",
                ]
                + parse_library_names(f"{current_dir}/build/lib"),
                extra_link_args=extra_link_args,
            )
        ],
        cmdclass={
            "build_ext": cpp_extension.BuildExtension,
        },
    )
