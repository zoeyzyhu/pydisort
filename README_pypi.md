<!-- Logo ------------------------------------------->
<p align="center">
  <img src="https://raw.githubusercontent.com/zoeyzyhu/pydisort/main/docs/img/logo_tr_git.png" alt="Pydisort" width="300">
</p>

<!-- Subtitle --------------------------------------->
<p align="center">
  <i align="center">Empower Discrete Ordinates Radiative Transfer (DISORT) with Python 🚀</i>
</p>

## <a id='about-pydisort'> About Pydisort </a>

A modern Python package for the DISORT (Discrete Ordinate Radiative Transfer) algorithm for efficient, high-precision modeling of radiative scattering and absorption in layered media.

`pydisort` provides a high-level Python API to the well-tested C implementation of DISORT, originally developed in Fortran (Stamnes et al. 1988) and later ported to C as `cdisort` by Timothy E. Dowling, which is a critical component of `libRadTran`. To support Python integration, the C code was first encapsulated in C++ classes, which were then exposed to Python using `pybind11`. For efficient memory management and potential GPU acceleration, `pydisort` leverages `PyTorch` tensors, paving the way for future applications in machine learning and large-scale parallel computation.

The normal usage of pydisort is to create a `pydisort.DisortOptions` object first and then initialize the `pydisort.cpp.Disort` object with the `pydisort.DisortOptions` object by:

```python
>>> import torch
>>> from pydisort import DisortOptions, Disort
>>> op = DisortOptions().flags("onlyfl,lamber")
>>> op.ds().nlyr = 4
>>> op.ds().nstr = 4
>>> op.ds().nmom = 4
>>> op.ds().nphase = 4
>>> ds = Disort(op)
>>> tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).unsqueeze(-1)
>>> flx = ds.forward(tau, fbeam=torch.tensor([3.14159]))
>>> flx
tensor([[[[0.0000, 3.1416],
        [0.0000, 2.8426],
        [0.0000, 2.3273],
        [0.0000, 1.7241],
        [0.0000, 1.1557]]]])
```

For a detailed documentation, please visit https://pydisort.readthedocs.io/.
