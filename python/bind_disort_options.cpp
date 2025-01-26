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
        Set radiation flags and dimension for disort

        Returns
        -------
        DisortOption object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.DisortOptionis().header('Test run').flags('onlyfl').nwave(10).ncol(10)
        >>> op.ds().nlyr, op.ds().nstr, op.ds().nmom = 10, 4, 4
        >>> print(op)
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

        A General boundary condition is invoked when 'ibcnd' is unspecified (False).
        This allows:

        - beam illumination from the top (set fbeam)
        - isotropic illumination from the top (set fisot)
        - thermal emission from the top (set ttemp and temis)
        - internal thermal emission (use set_temperature_on_level)
        - reflection at the bottom (set lamber, albedo)
        - thermal emission from the bottom (set btemp)

        A Special boundary condition is invoked when 'ibcnd' is specified (True).
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
      .ADD_OPTION(disort_state, disort::DisortOptions, ds, R"(
        Set disort state for disort

        Parameters
        ----------
        ds : disort_state
            disort state for disort

        Returns
        -------
        DisortOptions object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.DisortOptions()
        >>> op.ds().nlyr, op.ds().nstr, op.ds().nmom = 10, 4, 4
        >>> print(op)
        )")

      .ADD_OPTION(std::string, disort::DisortOptions, header, R"(
        Set header for disort

        Parameters
        ----------
        header : str
            header for disort

        Returns
        -------
        DisortOptions object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.DisortOptions().header('Test run')
        >>> print(op)
        )")

      .ADD_OPTION(std::string, disort::DisortOptions, flags, R"(
        Set radiation flags for disort

        Parameters
        ----------
        flags : str
            radiation flags for disort

        Returns
        -------
        DisortOptions object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.DisortOptions().flags('onlyfl')
        >>> print(op)
        )")

      .ADD_OPTION(int, disort::DisortOptions, nwave, R"(
        Set number of wavelengths for disort

        Parameters
        ----------
        nwave : int
            number of wavelengths for disort

        Returns
        -------
        DisortOptions object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.DisortOptions().nwave(10)
        >>> print(op)
        )")

      .ADD_OPTION(int, disort::DisortOptions, ncol, R"(
        Set number of columns for disort

        Parameters
        ----------
        ncol : int
            number of columns for disort

        Returns
        -------
        DisortOptions object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.DisortOptions().ncol(10)
        >>> print(op)
        )")

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, user_tau, R"(
        Set user optical depths for disort

        Parameters
        ----------
        user_tau : list
            user optical depths for disort

        Returns
        -------
        DisortOptions object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.DisortOptions().user_tau([0.1, 0.2, 0.3])
        >>> print(op)
        )")

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, user_mu, R"(
        Set user zenith angles for disort

        Parameters
        ----------
        user_mu : list
            user zenith angles for disort

        Returns
        -------
        DisortOptions object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.DisortOptions().user_mu([0.1, 0.2, 0.3])
        >>> print(op)
        )")

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, user_phi, R"(
        Set user azimuthal angles for disort

        Parameters
        ----------
        user_phi : list
            user azimuthal angles for disort

        Returns
        -------
        DisortOptions object

        Examples
        --------
        >>> import pydisort
        >>> op = pydisort.DisortOptions().user_phi([0.1, 0.2, 0.3])
        >>> print(op)
        )");
}
