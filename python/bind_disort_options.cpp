// torch
#include <torch/extension.h>

// python
#include "pyoptions.hpp"

// disort
#include <disort/disort.hpp>
#include <disort/disort_formatter.hpp>

namespace py = pybind11;

void bind_disort_options(py::module &m) {
  py::class_<disort::DisortOptions>(m, "DisortOptions", R"(
        Set radiation flags for disort

        Parameters
        ----------
        arg0 : dict
          Dictionary of radiation flags consisting of a key (flag) and a value (True/False).

        Returns
        -------
        DisortWrapper object

        Examples
        --------
        >>> import pydisort
        >>> disort = pydisort.disort()
        >>> dict = {'ibcnd': False, 'usrtau': True, 'usrang': True, 'lamber': True, 'plank': True}
        >>> disort.set_flags(dict).seal()
        >>> rad, flx = disort.run()

        Notes
        -----
        The following flags are supported:

        .. list-table::
           :widths: 25 25
           :header-rows: 1

           * - Flag
             - Description
           * - 'ibcnd'
             - General or Specific boundary condition
           * - 'usrtau'
             - use user optical depths
           * - 'usrang'
             - use user azimuthal angles
           * - 'lamber'
             - turn on lambertian reflection surface
           * - 'plank'
             - turn on plank source (thermal emission)
           * - 'spher'
             - turn on spherical correction
           * - 'onlyfl'
             - only compute radiative fluxes
           * - 'quiet'
             - turn on disort internal printout
           * - 'intensity_correction'
             - turn on intensity correction
           * - 'old_intensity_correction'
             - turn on old intensity correction
           * - 'general_source'
             - turn on general source
           * - 'output_uum'
             - output azimuthal components of the intensity
           * - 'print-input'
             - print input parameters
           * - 'print-fluxes'
             - print fluxes
           * - 'print-intensity'
             - print intensity
           * - 'print-transmissivity'
             - print transmissivity
           * - 'print-phase-function'
             - print phase function

        A General boundary condition is invoked when 'ibcnd' is set to False.
        This allows:

        - beam illumination from the top (set fbeam)
        - isotropic illumination from the top (set fisot)
        - thermal emission from the top (set ttemp and temis)
        - internal thermal emission (use set_temperature_on_level)
        - reflection at the bottom (set lamber, albedo)
        - thermal emission from the bottom (set btemp)

        A Special boundary condition is invoked when 'ibcnd' is set to True.
        Special boundary condition only returns albedo and transmissivity of
        the entire medium.

        - current version of pydisort has limited support for this option.
        - consult the documentation of DISORT for more details on this option.
        )")
      .def(py::init<>())
      .def("__repr__",
           [](const disort::DisortOptions &a) {
             return fmt::format("DisortOptions{}", a);
           })
      .ADD_OPTION(disort_state, disort::DisortOptions, ds)
      .ADD_OPTION(std::string, disort::DisortOptions, header)
      .ADD_OPTION(std::string, disort::DisortOptions, flags)
      .ADD_OPTION(int, disort::DisortOptions, nwave)
      .ADD_OPTION(int, disort::DisortOptions, ncol)
      .ADD_OPTION(std::vector<double>, disort::DisortOptions, user_tau)
      .ADD_OPTION(std::vector<double>, disort::DisortOptions, user_mu)
      .ADD_OPTION(std::vector<double>, disort::DisortOptions, user_phi);
}
