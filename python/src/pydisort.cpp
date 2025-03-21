// torch
#include <torch/extension.h>

// fmt
#include <fmt/format.h>

// python
#include "pyoptions.hpp"

// disort
#include <disort/disort.hpp>
#include <disort/disort_formatter.hpp>

namespace py = pybind11;

void bind_disort_options(py::module &m);
void bind_phase_options(py::module &m);
void bind_cdisort(py::module &m);

PYBIND11_MODULE(pydisort, m) {
  m.attr("__name__") = "disort";
  m.doc() = R"(
  Python bindings for DISORT (Discrete Ordinates Radiative Transfer) Program.

  Summary
  -------
  This module provides a python interface to the C version of the DISORT program.
  Please consult the DISORT publication [1]_ for more information on the DISORT program,
  and the C-DISORT C publication [2]_ for more information on the C version of the DISORT program.

  Small changes have been made to the C-DISORT program to make it compatible with python scripting.
  The C-DISORT program has been wrapped first in a C++ class (DisortWrapper), and the C++ class
  has been bound to python using pybind11.

  Pydisort features the following benefits over the original C-DISORT program:

  - Proper handling of errors rather than abrupt exit of the program. Errors
    can be caught and and handled in the python script.
  - Memory management is handled by the C++ class. The user does not need to
    worry about memory allocation and deallocation.
  - Documentation is automated using sphinx and readthedocs.
  - Safety guards are implemented to prevent the user from setting incorrect
    values for arrays or calling methods in the wrong order.

  Note that the underlying calculation engine is still the same as the C-DISORT program.
  So the speed of pydisort is the same as the origin C-DISORT program.

  Examples
  --------
  - Example 1: Calculate attenuation of radiative flux in a plane-parallel atmosphere

  >>> import torch
  >>> from pydisort import DisortOptions, Disort
  >>> op = DisortOptions().flags("onlyfl,lamber")
  >>> op.ds().nlyr = 4
  >>> op.ds().nstr = 4
  >>> op.ds().nmom = 4
  >>> op.ds().nphase = 4
  >>> ds = Disort(op)
  >>> tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).reshape((4,1))
  >>> bc = {"fbeam" : torch.tensor([3.14159]).reshape((1,1))}
  >>> flx = ds.forward(tau, bc)
  >>> flx
  tensor([[[[0.0000, 3.1416],
          [0.0000, 2.8426],
          [0.0000, 2.3273],
          [0.0000, 1.7241],
          [0.0000, 1.1557]]]])

  It is important to understand the dimensions of the input and output arrays.
  The input array `tau` has two dimensions. In order of appearance, they are:
    (1) The layer dimension (nlyr = 4),
    (2) The property dimension (nprop = 1).

  Since this problem only has optical thickness, the property dimension is 1.
  If not specified, both the wavelength/wavenumber dimension and the column dimension
  are assumed to be 1 and are automatically added internally to the input array.

  The boundary condition dictionary `bc` has one key, `fbeam`, which is the solar beam flux.
  The key `fbeam` has two dimensions. In order of appearance, they are:
    (1) The wavelength/wavenumber dimension (nwave = 1),
    (2) The column dimension (ncol = 1).

  In the example above, flx has four dimensions. In order of appearance, they are:
    (1) The wavelenth/wavenumber dimension (nwave = 1),
    (2) The column dimension (ncol = 1),
    (3) The level dimension (nlvl = nlyr + 1 = 5),
    (4) The flux field dimension (nflx = 2). The first element is upward flux,
        and the second element is downward flux.
  The attenuation of radiative flux is according to the Beer-Lambert law, i.e.,
  The example code above is in `test_attenuation.py`.

  .. math::

    F(z) = F(0) \exp(-\tau(z)),

  where :math:`F(z)` is the radiative flux at level :math:`z`,
  :math:`F(0)` is the radiative flux at the top of the atmosphere, and :math:`\tau(z)` is the
  optical depth from the top of the atmosphere to level :math:`z`. The default direction of
  radiative flux is nadir.


  Troubleshooting
  ---------------
  - The most common error is "RuntimeError: DisortImport::forward", which indicates
    that the disort run has failed. This error is mostly due to incorrect input
    dimensions or values. The error message shall provide more information on the
    cause of the error.

  - The program should not exit unexpectedly. If the program exits unexpectedly,
    please report the issue to the author (zoey.zyhu@gmail.com).

  Tips
  ----
  - Number of atmosphere levels is one more than the number of atmosphere layers.

  - Temperature is defined on levels, not layers. Other properties such as
    optical thickness, single scattering albedo, and phase function moments
    are defined on layers.

  - You can use ``print()`` method to print some of the DISORT internal states.

  - If you want to have more insights into DISORT internal inputs,
    you can set the ``print-input`` flag to ``True``.
    The DISORT internal inputs will be printed to the standard output
    when the ``forward()`` method is called.

  References
  ----------
  .. [1] Stamnes, K., Tsay, S. C., Wiscombe, W., & Jayaweera, K. (1988).
         Numerically stable algorithm for discrete-ordinate-method radiative transfer in multiple scattering and emitting layered media.
         Applied Optics, 27(12), 2502-2509.
  .. [2] Buras, R., & Dowling, T. (1996).
         Discrete-ordinate-method for radiative transfer in planetary atmospheres: Generalization of the doubling and adding method.
         Journal of Quantitative Spectroscopy and Radiative Transfer, 55(6), 761-779.
  )";

  m.attr("irfldir") = 0;
  m.attr("ifldn") = 1;
  m.attr("iflup") = 2;
  m.attr("idfdt") = 3;
  m.attr("iuavg") = 4;
  m.attr("iuavgdn") = 5;
  m.attr("iuavgup") = 6;
  m.attr("iuavgso") = 7;

  bind_disort_options(m);
  bind_phase_options(m);
  bind_cdisort(m);

  m.def("scattering_moments", &disort::scattering_moments, R"(
      Get phase function moments based on a phase function model

      Parameters
      ----------
      nmom : int
          Number of phase function moments
      op : PhaseMomentOptions
          Phase function model.

      Returns
      -------
      pmom : List[float]
          Phase function moments, shape (nmom,)
      )");

  ADD_DISORT_MODULE(Disort, DisortOptions)
      .def_readonly("options", &disort::DisortImpl::options)
      .def("gather_flx", &disort::DisortImpl::gather_flx, R"(
        Gather all disort flux outputs

        Returns
        -------
        torch.Tensor
            Disort flux outputs (nwave, ncol, nlvl = nlyr + 1, 8)
        )")

      .def("gather_rad", &disort::DisortImpl::gather_rad, R"(
        Gather all disort radiation outputs

        Returns
        -------
        torch.Tensor
            Disort radiation outputs (nwave, ncol, nlvl = nlyr + 1, 6)
        )")

      .def(
          "forward",
          [](disort::DisortImpl &self, torch::Tensor prop,
             std::map<std::string, torch::Tensor> &bc, std::string bname,
             torch::optional<torch::Tensor> temf) {
            while (prop.dim() < 4) {  // (nwave, ncol, nlyr, nprop)
              prop = prop.unsqueeze(0);
            }
            return self.forward(prop, &bc, bname, temf);
          },
          R"(
        Calculate radiative flux or intensity

        Parameters
        ----------
        prop : torch.Tensor
            Optical properties at each level (nwave, ncol, nlyr, nprop)
        bc : Dict[str, torch.Tensor]
            Dictionary of disort boundary conditions
            The dimensions of each recognized key are:
            - <band> + "umu0" : (ncol,), cosine of solar zenith angle
            - <band> + "phi0" : (ncol,), azimuthal angle of solar beam
            - <band> + "fbeam" : (nwave, ncol), solar beam flux
            - <band> + "albedo" : (nwave, ncol), surface albedo
            - <band> + "fluor" : (nwave, ncol), isotropic bottom illumination
            - <band> + "fisot" : (nwave, ncol), isotropic top illumination
            - <band> + "temis" : (nwave, ncol), top emissivity
            - "btemp" : (ncol,), bottom temperature
            - "ttemp" : (ncol,), top temperature

            Some keys can have a prefix band name, <band>.
            If the prefix is an non-empty string, a slash "/" is
            automatically appended to it, such that the key look like
            `B1/umu0`. `btemp` and `ttemp` do not have a band name prefix.
        bname : str
            Name of the radiation band
        temf : Optional[torch.Tensor]
            Temperature at each level (ncol, nlvl = nlyr + 1)

        Returns
        -------
        torch.Tensor
            Radiative flux or intensity (nwave, ncol, nlvl, nrad)
        )",
          py::arg("prop"), py::arg("bc"), py::arg("bname") = "",
          py::arg("temf") = py::none());
}
