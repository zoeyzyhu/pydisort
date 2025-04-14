import os
import signal
import sysconfig
import shutil
import atexit

site_packages_dir = sysconfig.get_path("purelib")

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
    #post_install_relink()

except ImportError:
    pass

atexit.register(cleanup)
signal.signal(signal.SIGINT, handle_exit)
signal.signal(signal.SIGTERM, handle_exit)

from .pydisort import *

__version__ = "0.0.1"
