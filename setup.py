import os
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
include_dirs = [
    f"{current_dir}",
    f"{current_dir}/build",
    f"{current_dir}/build/_deps/fmt-src/include",
]
lib_dirs = [f"{current_dir}/build/lib"]
libraries = parse_library_names(f"{current_dir}/build/lib")

if torch.cuda.is_available():
    ext_module = cpp_extension.CUDAExtension(
        name="pydisort.pydisort",
        sources=glob.glob("python/csrc/*.cpp")
        + glob.glob("src/**/*.cu", recursive=True),
        include_dirs=include_dirs,
        library_dirs=lib_dirs,
        libraries=libraries,
        extra_compile_args={"nvcc": ["--extended-lambda"]},
    )
else:
    ext_module = cpp_extension.CppExtension(
        name="pydisort.pydisort",
        sources=glob.glob("python/csrc/*.cpp"),
        include_dirs=include_dirs,
        library_dirs=lib_dirs,
        libraries=libraries,
    )

setup(
    package_dir={"pydisort": "python"},
    packages=["pydisort"],
    ext_modules=[ext_module],
    cmdclass={"build_ext": cpp_extension.BuildExtension},
)
