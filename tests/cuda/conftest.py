"""Shared setup for the CUDA solver tests.

pydisort's CUDA path is a separate implementation of the same solver: cdisort
is compiled as ``__host__ __device__`` code and one GPU thread solves one
(wave, column) element. It therefore needs its own correctness checks, and
those checks need a GPU.

Continuous integration has none, so everything in this directory skips when
``torch.cuda.is_available()`` is false.
"""

import pytest
import torch

# The CUDA solver is FP64 throughout; comparing against a float32 CPU run
# would measure the dtype rather than the device.
DTYPE = torch.float64

requires_cuda = pytest.mark.skipif(
    not torch.cuda.is_available(),
    reason="no CUDA device available",
)


@pytest.fixture(scope="session")
def cuda_device():
    return torch.device("cuda")


@pytest.fixture(scope="session")
def cpu_device():
    return torch.device("cpu")
