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

        This is usually the first step in setting up a disort run.
        Some disort options can be set directly in the ``disort_state`` object,
        such as the dimensions and the flags. Others, such as the polarj
        and azimuthal angles requires allocating the internal arrays of ``disort_state``.
        The ``DisortOptions`` object holds those arrays temporarily until the
        ``disort_state`` object is initialized when a ``Disort`` object
        is created based on the ``DisortOptions`` object.

        .. note::

          When the ``DisortOptions`` object is printed, it may not truly reflect
          the state of the ``disort_state`` object. This is because the
          ``DisortOptions`` object holds temporary arrays that are not
          yet transferred to the ``disort_state`` object. Transferring happens
          when the ``Disort`` object is created by calling:

          .. code-block:: python

            >>> disort = pydisort.Disort(op)

          where ``op`` is the ``DisortOptions`` object.

        Returns:
          DisortOption object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions().flags('onlyfl').nwave(10).ncol(10)
          >>> op.ds().nlyr, op.ds().nstr, op.ds().nmom = 10, 4, 4
          >>> print(op)
          DisortOptions(flags = onlyfl; nwave = 10; ncol = 10; disort_state = (nlyr = 10; nstr = 4; nmom = 4; ibcnd = 0; usrtau = 0; usrang = 0; lamber = 0; planck = 0; spher = 0; onlyfl = 0); wave = ())

        **The following flags are supported:**

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

            - beam illumination from the top
            - isotropic illumination from the top
            - thermal emission from the top
            - internal thermal emission
            - reflection at the bottom
            - thermal emission from the bottom

          A Special boundary condition is invoked when 'ibcnd' is specified (True).
          Special boundary condition only returns albedo and transmissivity of
          the entire medium.

          .. warning::

            - current version of pydisort has limited support for this option.
            - consult the documentation of DISORT for more details on this option.
        )")
      .def(py::init<>())

      .def("__repr__",
           [](const disort::DisortOptions &a) {
             return fmt::format("DisortOptions{}", a);
           })

      .ADD_OPTION(std::string, disort::DisortOptions, header, R"(
        Set or get header for disort

        Usage:
          - header() -> str
          - header(header: str) -> DisortOptions

        Args:
          header (str, optional): header for disort

        Returns:
          DisortOptions: object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions().header('Test run')
          >>> print(op)
        )")

      .ADD_OPTION(std::string, disort::DisortOptions, flags, R"(
        Set or get radiation flags for disort

        Usage:
          - flags() -> str
          - flags(key: str) -> DisortOptions

        Args:
          flags (str, optional): radiation flags for disort

        Returns:
          DisortOptions: object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions().flags('onlyfl')
          >>> print(op)
        )")

      .ADD_OPTION(int, disort::DisortOptions, nwave, R"(
        Set or get number of wavelengths for disort

        Usage:
          - nwave() -> int
          - nwave(nwave: int) -> DisortOptions

        Args:
          nwave (int, optional): number of wavelengths for disort

        Returns:
          DisortOptions: object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions().nwave(10)
          >>> print(op)
        )")

      .ADD_OPTION(int, disort::DisortOptions, ncol, R"(
        Set or get number of columns for disort

        Usage:
          - ncol() -> int
          - ncol(ncol: int) -> DisortOptions

        Args:
          ncol (int, optional): number of columns for disort

        Returns:
          DisortOptions: object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions().ncol(10)
          >>> print(op)
        )")

      .ADD_OPTION(double, disort::DisortOptions, accur, R"(
        Set or get accuracy for disort

        Usage:
          - accur() -> float
          - accur(accur: float) -> DisortOptions

        Args:
          accur (float, optional): accuracy for disort

        Returns:
          DisortOptions: object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions().accur(1.e-6)
          >>> print(op)
        )")

      .ADD_OPTION(int, disort::DisortOptions, upward, R"(
        Set or get direction for disort

        Usage:
          - upward() -> int
          - upward(upward: int) -> DisortOptions

        Args:
          upward (int): direction for disort

        Returns:
          DisortOptions: object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions().upward(true)
          >>> print(op)
        )")

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, user_tau, R"(
        Set or get user optical depths for disort

        Usage:
          - user_tau() -> list
          - user_tau(user_tau: list) -> DisortOptions

        Args:
          user_tau (list): user optical depths for disort

        Returns:
          DisortOptions: object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions().user_tau([0.1, 0.2, 0.3])
          >>> print(op)
        )")

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, user_mu, R"(
        Set or get user zenith angles for disort

        Usage:
          - user_mu() -> list
          - user_mu(user_mu: list) -> DisortOptions

        Args:
          user_mu (list): user zenith angles for disort

        Returns:
          DisortOptions: object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions().user_mu([0.1, 0.2, 0.3])
          >>> print(op)
        )")

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, user_phi, R"(
        Set or get user azimuthal angles for disort

        Usage:
          - user_phi() -> list
          - user_phi(user_phi: list) -> DisortOptions

        Args:
          user_phi (list): user azimuthal angles for disort

        Returns:
          DisortOptions: object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions().user_phi([0.1, 0.2, 0.3])
          >>> print(op)
        )")

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, wave_lower, R"(
        Set or get lower wavenumber(length) at each bin for disort

        Usage:
          - wave_lower() -> list
          - wave_lower(wave_lower: list) -> DisortOptions

        Args:
          wave_lower (list): lower wavenumber(length) at each bin for disort

        Returns:
          DisortOptions: object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions().wave_lower([0.1, 0.2, 0.3])
          >>> print(op)
        )")

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, wave_upper, R"(
        Set or get upper wavenumber(length) at each bin for disort

        Usage:
          - wave_upper() -> list
          - wave_upper(wave_upper: list) -> DisortOptions

        Args:
          wave_upper (list): upper wavenumber(length) at each bin for disort

        Returns:
          DisortOptions: object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions().wave_upper([0.1, 0.2, 0.3])
          >>> print(op)
        )")

      .ADD_OPTION(disort_state, disort::DisortOptions, ds, R"(
        Set disort state for disort

        Args:
          ds (disort_state): disort state for disort

        Returns:
          DisortOptions: object

        Examples:

        .. code-block:: python

          >>> import pydisort
          >>> op = pydisort.DisortOptions()
          >>> op.ds().nlyr, op.ds().nstr, op.ds().nmom = 10, 4, 4
          >>> print(op)
        )");
}
