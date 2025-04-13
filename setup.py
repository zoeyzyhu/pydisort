#!/usr/bin/env python
import os
import sysconfig
import platform
import glob
import torch
from pathlib import Path
from setuptools import setup
from torch.utils import cpp_extension


def parse_library_names(libdir):
    library_names = []
    for root, _, files in os.walk(libdir):
        for file in files:
            if file.endswith((".a", ".so")):
                file_name = os.path.basename(file)
                library_names.append(file_name[3:].rsplit(".", 1)[0])
    return library_names


current_dir = os.getenv("WORKSPACE", Path().absolute())
site_packages_dir = sysconfig.get_path("purelib")
torch_lib_dir = os.path.join(os.path.dirname(torch.__file__), "lib")
torch_include_dir = torch.utils.cpp_extension.include_paths()

include_dirs = [
    f"{current_dir}",
    f"{current_dir}/build",
    f"{current_dir}/build/_deps/fmt-src/include",
] + torch_include_dir

lib_dirs = [
    f"{current_dir}/build/lib",
    torch_lib_dir,
    site_packages_dir,
]

libraries = ["torch_global_deps"] + parse_library_names(
    f"{current_dir}/build/lib"
)


extra_link_args = []
if platform.system() == "Darwin":
    extra_link_args += [
        f"-Wl,-rpath,{torch_lib_dir}",
        "-Wl,-rpath,@loader_path/.dylibs",
        "-Wl,-rpath,@executable_path/.dylibs",
    ]
else:
    extra_link_args += [
        f"-Wl,-rpath,{torch_lib_dir}",
        "-Wl,-rpath,$ORIGIN/.libs",
    ]

if torch.cuda.is_available():
    ext_module = cpp_extension.CUDAExtension(
        name="pydisort.pydisort",
        sources=glob.glob("python/csrc/*.cpp")
        + glob.glob("src/**/*.cu", recursive=True),
        include_dirs=include_dirs,
        library_dirs=lib_dirs,
        libraries=libraries,
        extra_compile_args={"nvcc": ["--extended-lambda"]},
        # extra_link_args=extra_link_args,
    )
else:
    ext_module = cpp_extension.CppExtension(
        name="pydisort.pydisort",
        sources=glob.glob("python/csrc/*.cpp"),
        include_dirs=include_dirs,
        library_dirs=lib_dirs,
        libraries=libraries,
        # extra_link_args=extra_link_args,
    )

setup(
    package_dir={"pydisort": "python"},
    packages=["pydisort"],
    ext_modules=[ext_module],
    cmdclass={"build_ext": cpp_extension.BuildExtension},
)
