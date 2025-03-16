// torch
#include <torch/extension.h>

// python
#include "pyoptions.hpp"

// disort
#include <disort/disort_formatter.hpp>
#include <disort/scattering_moments.hpp>

namespace py = pybind11;

void bind_phase_options(py::module &m) {
  py::class_<disort::PhaseMomentOptions>(m, "PhaseMomentOptions")
      .def(py::init<>())
      .def("__repr__",
           [](const disort::PhaseMomentOptions &a) {
             return fmt::format("PhaseMomentOptions{}", a);
           })
      .def(
          "type",
          [](disort::PhaseMomentOptions &a, std::string type) {
            if (type == "isotropic") {
              a.type(disort::kIsotropic);
            } else if (type == "rayleigh") {
              a.type(disort::kRayleigh);
            } else if (type == "henyey-greenstein") {
              a.type(disort::kHenyeyGreenstein);
            } else if (type == "double-henyey-greenstein") {
              a.type(disort::kDoubleHenyeyGreenstein);
            } else if (type == "haze-garcia-siewert") {
              a.type(disort::kHazeGarciaSiewert);
            } else if (type == "cloud-garcia-siewert") {
              a.type(disort::kCloudGarciaSiewert);
            } else {
              throw std::runtime_error("Unknown phase function model");
            }

            return a;
          },
          R"(
      Notes
      -----
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
      )")

      .ADD_OPTION(double, disort::PhaseMomentOptions, gg, R"(
        Set the asymmetry parameter for the Henyey-Greenstein phase function

        Parameters
        ----------
        gg : float
            Asymmetry parameter

        Returns
        -------
        PhaseMomentOptions object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.PhaseMomentOptions().type('henyey-greenstein').gg(0.85)
        >>> print(op)
        )")

      .ADD_OPTION(double, disort::PhaseMomentOptions, gg1, R"(
        Set the asymmetry parameter for the first Henyey-Greenstein phase function

        Parameters
        ----------
        gg1 : float
            Asymmetry parameter

        Returns
        -------
        PhaseMomentOptions object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.PhaseMomentOptions().type('double-henyey-greenstein')
        >>> op.gg1(0.85).gg2(0.6).ff(0.85)
        >>> print(op)
        )")

      .ADD_OPTION(double, disort::PhaseMomentOptions, gg2, R"(
        Set the asymmetry parameter for the second Henyey-Greenstein phase function

        Parameters
        ----------
        gg2 : float
            Asymmetry parameter

        Returns
        -------
        PhaseMomentOptions object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.PhaseMomentOptions().type('double-henyey-greenstein')
        >>> op.gg1(0.85).gg2(0.6).ff(0.85)
        >>> print(op)
        )")

      .ADD_OPTION(double, disort::PhaseMomentOptions, ff, R"(
        Set the forward scattering fraction for the Henyey-Greenstein phase function

        Parameters
        ----------
        ff : float
            Forward scattering fraction

        Returns
        -------
        PhaseMomentOptions object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.PhaseMomentOptions().type('double-henyey-greenstein')
        >>> op.gg1(0.85).gg2(0.6).ff(0.85)
        >>> print(op)
        )");
}
