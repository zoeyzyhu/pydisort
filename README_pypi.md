<!-- Logo ------------------------------------------->
<p align="center">
  <img src="https://raw.githubusercontent.com/zoeyzyhu/pydisort/main/docs/img/logo_tr_git.png" alt="Pydisort" width="300">
</p>

<!-- Subtitle --------------------------------------->
<p align="center">
  <i align="center">Empower Discrete Ordinates Radiative Transfer (DISORT) with Python 🚀</i>
</p>

## <a id='about-pydisort'> About Pydisort </a>

DISORT (Discrete Ordinate Radiative Transfer) is a widely-used algorithm that calculates the scattering and absorption of radiation in a medium. The original DISORT algorithm was developed by Stamnes et al. in 1988 and was implemented in `FORTRAN`. `pydisort` is a Python wrapper for the DISORT algorithm in `C` by Timothy E. Dowling. Small changes have been made to the `cdisort` package (an important component of the `libRadTran` software) to make it compatible with python scripting. The `cdisort` code has been wrapped first in C++ classes, and the C++ classes have been bound to python using `pybind11`. To enable automatic parallelization, `pydisort` uses PyTorch tensors structure for memory management, and potentially for GPU acceleration and machine learning compatibility in the future.

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
