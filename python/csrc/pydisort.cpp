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
  m.attr("__name__") = "pydisort";
  m.doc() = R"(
Python bindings for DISORT (Discrete Ordinates Radiative Transfer) Program.
  )";

  m.attr("irfldir") = 0;
  m.attr("ifldn") = 1;
  m.attr("iflup") = 2;
  m.attr("idfdt") = 3;
  m.attr("iuavg") = 4;
  m.attr("iuavgdn") = 5;
  m.attr("iuavgup") = 6;
  m.attr("iuavgso") = 7;

  bind_cdisort(m);
  bind_disort_options(m);

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
    - Double Henyey-Greenstein phase function,
      [ff*gg1 + (1-ff)*gg2, ff*gg1^2 + (1-ff)*gg2^2, ...]
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
    >>> tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).unsqueeze(-1)
    >>> flx = ds.forward(tau, fbeam=torch.tensor([3.14159]))
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
    >>>   "albedo": torch.tensor([0.0]),
    >>>   "fluor": torch.tensor([0.0]),
    >>>   "fisot": torch.tensor([0.0]),
    >>> }
    >>> bc["fbeam"] = np.pi / bc["umu0"]
    >>> tau = torch.zeros((ncol, nprop))
    >>> tau[0, 0] = ds.options.user_tau()[-1]
    >>> tau[0, 1] = 0.2
    >>> tau[0, 2:] = scattering_moments(nprop - 2, "isotropic")
    >>> flx = ds.forward(tau, **bc)
    >>> ds.gather_rad()
    tensor([[[[[0.0000, 0.0000, 0.0000, 0.1178, 0.0264, 0.0134],
               [0.0134, 0.0263, 0.1159, 0.0000, 0.0000, 0.0000]]]]])
        )")

      .def(
          "forward",
          [](disort::DisortImpl &self, torch::Tensor prop, std::string bname,
             torch::optional<torch::Tensor> temf, const py::kwargs &kwargs) {
            // get bc from kwargs
            std::map<std::string, torch::Tensor> bc;
            for (auto item : kwargs) {
              auto key = py::cast<std::string>(item.first);
              auto value = py::cast<torch::Tensor>(item.second);
              bc.emplace(std::move(key), std::move(value));
            }

            for (auto &[key, value] : bc) {
              std::vector<std::string> items = {"fbeam", "albedo", "fluor",
                                                "fisot", "temis"};

              // broadcast dimensions to (nwave, ncol)
              if (std::find(items.begin(), items.end(), key) != items.end()) {
                while (value.dim() < 2) {
                  value = value.unsqueeze(0);
                }
              }
            }

            // broadcast dimensions to (nwave, ncol, nlyr, nprop)
            while (prop.dim() < 4) {
              prop = prop.unsqueeze(0);
            }

            return self.forward(prop, &bc, bname, temf);
          },
          py::arg("prop"), py::arg("bname") = "", py::arg("temf") = py::none(),
          R"(
Calculate radiative flux or intensity

The dimensions of each recognized key in ``kwargs`` are:

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

Some keys can have a prefix band name, ``<band>``. If the prefix is an non-empty string,
a slash "/" is automatically appended to it, such that the key looks like ``B1/umu0``.
``btemp`` and ``ttemp`` do not have a band name prefix.
If the values are short of wave or column dimensions, they are automatically broadcasted to be the shape of 1.

Args:
  prop (torch.Tensor): Optical properties at each level (nwave, ncol, nlyr, nprop)
  bname (str): Name of the radiation band, default is empty string.
    If the name is not empty, a slash "/" is automatically appended to it.
  temf (Optional[torch.Tensor]): Temperature at each level (ncol, nlvl = nlyr + 1),
    default is None. If not None, the temperature is used to calculate the Planck function.
  kwargs (Dict[str, torch.Tensor]): keyword arguments of disort boundary conditions, see keys listed above

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
    >>> tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).unsqueeze(-1)
    >>> flx = ds.forward(tau, fbeam=torch.tensor([3.14159]))
    >>> flx
    tensor([[[[0.0000, 3.1416],
            [0.0000, 2.8426],
            [0.0000, 2.3273],
            [0.0000, 1.7241],
            [0.0000, 1.1557]]]])
        )");
}
