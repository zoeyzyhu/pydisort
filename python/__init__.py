import os
import signal
import sysconfig
import shutil
import atexit

site_packages_dir = sysconfig.get_path("purelib")

def add_dylib_path():
    lib_path_str = f"{site_packages_dir}/pydisort/lib"

    if platform.system() == "Darwin":
        varname = "DYLD_LIBRARY_PATH"
    elif platform.system() == "Linux":
        varname = "LD_LIBRARY_PATH"
    else:
        varname = None  # Windows would use PATH, but usually needs a different strategy

    if varname:
        existing = os.environ.get(varname, "")
        if lib_path_str not in existing.split(":"):
            os.environ[varname] = lib_path_str + (":" + existing if existing else "")

def cleanup():
    link_path = os.path.join(site_packages_dir, "pydisort", ".dylibs")
    if os.path.exists(link_path) and os.path.islink(link_path):
        os.unlink(link_path)


def handle_exit(sig, frame):
    cleanup()


def post_install_relink():
    # locations
    torch_path = os.path.join(site_packages_dir, "torch", "lib")
    link_path = os.path.join(site_packages_dir, "pydisort", ".dylibs")

    if os.path.exists(link_path):
        # Check if the link is valid
        if os.path.islink(link_path):
            target = os.readlink(link_path)
            if target == torch_path:
                return
            os.unlink(link_path)
        else:
            # If it's not a symlink, remove the folder
            shutil.rmtree(link_path)

    # Now create the symlink
    os.makedirs(os.path.dirname(link_path), exist_ok=True)
    os.symlink(torch_path, link_path)


# relink libraries
try:
    import torch

    post_install_relink()

except ImportError:
    pass

atexit.register(cleanup)
signal.signal(signal.SIGINT, handle_exit)
signal.signal(signal.SIGTERM, handle_exit)

add_dylib_path()

from .pydisort import *

__version__ = "1.2.11"
