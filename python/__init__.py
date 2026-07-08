from importlib.metadata import PackageNotFoundError, version

import torch
from .pydisort import *

try:
    __version__ = version("pydisort")
except PackageNotFoundError:
    __version__ = "0.0.0"
