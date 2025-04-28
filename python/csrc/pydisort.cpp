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

  The normal usage of pydisort is to create a :class:`pydisort.DisortOptions` object first
  and then initialize the :class:`pydisort.Disort <disort.cpp.Disort>` object with
  the :class:`pydisort.DisortOptions` object by:

  .. code-block:: python

    >>> import pydisort
    >>> op = pydisort.DisortOptions().flags("onlyfl,lamber")
    >>> op.ds().nlyr = 4
    >>> op.ds().nstr = 4
    >>> op.ds().nmom = 4
    >>> op.ds().nphase = 4
    >>> ds = pydisort.Disort(op)

  Examples
  --------
  - Example 1: Calculate attenuation of radiative flux in a plane-parallel atmosphere

  .. code-block:: python

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

    #. The layer dimension (nlyr = 4),
    #. The property dimension (nprop = 1).

  Since this problem only has optical thickness, the property dimension is 1.
  If not specified, both the wavelength/wavenumber dimension and the column dimension
  are assumed to be 1 and are automatically added internally to the input array.

  The boundary condition dictionary `bc` has one key, `fbeam`, which is the solar beam flux.
  The key `fbeam` has two dimensions. In order of appearance, they are:

    #. The wavelength/wavenumber dimension (nwave = 1),
    #. The column dimension (ncol = 1).

  In the example above, flx has four dimensions. In order of appearance, they are:

    #. The wavelenth/wavenumber dimension (nwave = 1),
    #. The column dimension (ncol = 1),
    #. The level dimension (nlvl = nlyr + 1 = 5),
    #. The flux field dimension (nflx = 2). The first element is upward flux, and the second element is downward flux.

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
  - The most common error is "RuntimeError: DisortImpl::forward", which indicates
    that the disort run has failed. This error is mostly due to incorrect input
    dimensions or values. The error message shall provide more information on the
    cause of the error.

  - The program should not exit unexpectedly. If the program exits unexpectedly,
    please report the issue to the author (zoey.zyhu@gmail.com).

  .. tip::

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
  bind_cdisort(m);

  m.def("scattering_moments", &disort::scattering_moments, py::arg("nmom"),
        py::arg("type"), py::arg("gg1") = 0.0, py::arg("gg2") = 0.0,
        py::arg("ff") = 0.0, R"(
      Get phase function moments based on a phase function model

      The following phase function models are supported:

      .. list-table::
          :widths: 25 40
          :header-rows: 1

          * - Model
            - Description
          * - 'isotropic'
            - Isotropic phase function, [0, 0, 0, ...]
          * - 'rayleigh'
            - Rayleigh scattering phase function, [0, 0.1, 0, ...]
          * - 'henyey-greenstein'
            - Henyey-Greenstein phase function, [gg, gg^2, gg^3, ...]
          * - 'double-henyey-greenstein'
            - Double Henyey-Greenstein phase function, [gg1, gg2, gg1^2, gg2^2, ...]
          * - 'haze-garcia-siewert'
            - Tabulated haze phase function by Garcia/Siewert
          * - 'cloud-garcia-siewert'
            - Tabulated cloud phase function by Garcia/Siewert

      Args:
        nmom (int): Number of phase function moments
        type (str): Phase function model
        gg1 (float): First Henyey-Greenstein parameter
        gg2 (float): Second Henyey-Greenstein parameter
        ff (float): Weighting factor for double Henyey-Greenstein

      Returns:
        torch.Tensor: Phase function moments, shape (nmom,)

      Examples:
        Example 1: Isotropic phase function

        .. code-block:: python

          >>> import pydisort
          >>> pydisort.scattering_moments(4, 'isotropic')
          tensor([0., 0., 0., 0.], dtype=torch.float64)

        Example 2: Henyey-Greenstein phase function

        .. code-block:: python

          >>> import pydisort
          >>> pydisort.scattering_moments(4, 'henyey-greenstein', 0.85)
          tensor([0.8500, 0.7225, 0.6141, 0.5220], dtype=torch.float64)

        Example 3: Double Henyey-Greenstein phase function

        .. code-block:: python

          >>> import pydisort
          >>> pydisort.scattering_moments(4, 'double-henyey-greenstein', 0.85, 0.5, 0.5)
          tensor([0.6750, 0.4862, 0.3696, 0.2923], dtype=torch.float64)
      )");

  ADD_DISORT_MODULE(Disort, DisortOptions)
      .def_readonly("options", &disort::DisortImpl::options)
      .def("gather_flx", &disort::DisortImpl::gather_flx, R"(
        Gather all disort flux outputs

        Returns:
          torch.Tensor: Disort flux outputs (nwave, ncol, nlvl = nlyr + 1, 8)

        Examples:

          .. code-block:: python

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
            >>> ds.gather_flx()
        )")

      .def("gather_rad", &disort::DisortImpl::gather_rad, R"(
        Gather all disort radiation outputs

        Returns:
          torch.Tensor: Disort radiation outputs (nwave, ncol, nlvl = nlyr + 1, 6)

        Examples:

          .. code-block:: python

            >>> import torch
            >>> import numpy as np
            >>> from pydisort import DisortOptions, Disort, scattering_moments
            >>> op = DisortOptions().flags("usrtau,usrang,lamber,print-input")
            >>> op.ds().nlyr = 1
            >>> op.ds().nstr = 16
            >>> op.ds().nmom = 16
            >>> op.ds().nphase = 16
            >>> op.user_tau(np.array([0.0, 0.03125]))
            >>> op.user_mu(np.array([-1.0, -0.5, -0.1, 0.1, 0.5, 1.0]))
            >>> op.user_phi(np.array([0.0]))
            >>> nwave, ncol, nprop = 1, 1, 2 + op.ds().nmom
            >>> ds = Disort(op)
            >>> tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).reshape((4,1))
            >>> bc = {
            >>>   "umu0": torch.tensor([0.1]),
            >>>   "phi0": torch.tensor([0.0]),
            >>>   "albedo": torch.zeros((1, 1)),
            >>>   "fluor": torch.zeros((1, 1)),
            >>>   "fisot": torch.zeros((1, 1)),
            >>> }
            >>> bc["fbeam"] = np.pi / bc["umu0"].reshape((nwave, ncol))
            >>> tau = torch.zeros((ncol, nprop))
            >>> tau[0, 0] = ds.options.user_tau()[-1]
            >>> tau[0, 1] = 0.2
            >>> tau[0, 2:] = scattering_moments(nprop - 2, "isotropic")
            >>> flx = ds.forward(tau, bc)
            >>> ds.gather_rad()
            tensor([[[[[0.0000, 0.0000, 0.0000, 0.1178, 0.0264, 0.0134],
                       [0.0134, 0.0263, 0.1159, 0.0000, 0.0000, 0.0000]]]]])
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
          py::arg("prop"), py::arg("bc"), py::arg("bname") = "",
          py::arg("temf") = py::none(),
          R"(
        Calculate radiative flux or intensity

        The dimensions of each recognized key in ``bc`` are:

        .. list-table::
          :widths: 15 15 25
          :header-rows: 1

          * - Key
            - Shape
            - Description
          * - <band> + "umu0"
            - (ncol,)
            - cosine of solar zenith angle
          * - <band> + "phi0"
            - (ncol,)
            - azimuthal angle of solar beam
          * - <band> + "fbeam"
            - (nwave, ncol)
            - solar beam flux
          * - <band> + "albedo"
            - (nwave, ncol)
            - surface albedo
          * - <band> + "fluor"
            - (nwave, ncol)
            - isotropic bottom illumination
          * - <band> + "fisot"
            - (nwave, ncol)
            - isotropic top illumination
          * - <band> + "temis"
            - (nwave, ncol)
            - top emissivity
          * - "btemp"
            - (ncol,)
            - bottom temperature
          * - "ttemp"
            - (ncol,)
            - top temperature

        Some keys can have a prefix band name, ``<band>``. If the prefix is an non-empty string, a slash "/" is automatically appended to it, such that the key looks like ``B1/umu0``. ``btemp`` and ``ttemp`` do not have a band name prefix.

        Args:
          prop (torch.Tensor): Optical properties at each level (nwave, ncol, nlyr, nprop)

          bc (Dict[str, torch.Tensor]): Dictionary of disort boundary conditions.

          bname (str): Name of the radiation band

          temf (Optional[torch.Tensor]): Temperature at each level (ncol, nlvl = nlyr + 1)

        Returns:
          torch.Tensor: Radiative flux or intensity, shape (nwave, ncol, nlvl, nrad)

        Examples:
          .. code-block:: python

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
        )");
}
