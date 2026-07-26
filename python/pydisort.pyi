"""
Python bindings for DISORT (Discrete Ordinates Radiative Transfer) Program.

This stub file provides type hints and API documentation for the pydisort module.
"""

from typing import Any, Dict, List, Optional, Union, overload
import torch
import torch.nn as nn
from numpy import ndarray

# Module constants
kIRFLDIR: int
kIFLDN: int
kIFLUP: int
kIDFDT: int
kIUAVG: int
kIUAVGDN: int
kIUAVGUP: int
kIUAVGSO: int
kIEX: int
kISS: int
kIPM: int
kIUP: int
kIDN: int

class disort_state:
    """
    This is a wrapper for the ``disort_state`` object in the C DISORT library.
    The only important variables are:

    - ``nlyr``: number of layers
    - ``nstr``: number of streams
    - ``nmom``: number of phase function moments
    - ``nphase``: number of azimuthal angles

    The result of the variables will be transferred from the :class:`pydisort.DisortOptions`
    object when the :class:`pydisort.Disort <disort.cpp.Disort>` object is created.
    """

    nlyr: int
    """Number of layers"""

    nstr: int
    """Number of streams"""

    nmom: int
    """Number of phase functions moments"""

    nphase: int
    """Number of azimuthal angles"""

    def __init__(self) -> None:
        """
        Create a new default DISORT state object.

        Returns:
          pydisort.disort_state: class object

        Examples:
          >>> import pydisort
          >>> ds = pydisort.disort_state()
          >>> ds.nlyr, ds.nstr, ds.nphase = 10, 4, 4
          >>> print(ds)
          disort_state(nlyr = 10; nstr = 4; nmom = 0; ibcnd = 0; usrtau = 0; usrang = 0; lamber = 0; planck = 0; spher = 0; onlyfl = 0)
        """
        ...
    def __repr__(self) -> str: ...

class DisortOptions:
    """
    Set radiation flags and dimension for disort

    This is usually the first step in setting up a disort run.
    Some disort options can be set directly in the  :class:`pydisort.disort_state` object,
    such as the dimensions and the flags. Others, such as the polar and azimuthal angles requires
    allocating the internal arrays of :class:`pydisort.disort_state`.
    The :class:`pydisort.DisortOptions` object holds those arrays temporarily
    until the :class:`pydisort.disort_state` object is initialized when a
    :class:`pydisort.cpp.Disort` object is created based on
    the :class:`pydisort.DisortOptions` object.

    .. note::

      When the :class:`pydisort.DisortOptions` object is printed, it may not truly reflect
      the state of the :class:`pydisort.disort_state` object. This is because the
      :class:`pydisort.DisortOptions` object holds temporary arrays that are not
      yet transferred to the :class:`pydisort.disort_state` object. Transferring happens
      when the :class:`pydisort.cpp.Disort` object is created by calling:

      .. code-block:: python

        >>> disort = pydisort.Disort(op)

      where ``op`` is the :class:`pydisort.DisortOptions` object.

    Returns:
      pydisort.DisortOption: class object

    Examples:

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
         * - 'planck'
           - turn on planck source (thermal emission)
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
        - consult the `documentation <_static/DISORT2.doc>`_ of DISORT for more details on this option.
    """

    def __init__(self) -> None: ...
    def __repr__(self) -> str: ...
    @overload
    def header(self) -> str:
        """
        Get header for disort

        Returns:
          str: header for disort
        """
        ...
    @overload
    def header(self, header: str) -> DisortOptions:
        """
        Set header for disort

        Args:
          header (str): header for disort

        Returns:
          pydisort.DisortOptions: class object
        """
        ...
    @overload
    def flags(self) -> str:
        """
        Get radiation flags for disort

        Returns:
          str: radiation flags for disort
        """
        ...
    @overload
    def flags(self, flags: str) -> DisortOptions:
        """
        Set radiation flags for disort

        Args:
          flags (str): radiation flags for disort

        Returns:
          pydisort.DisortOptions: class object
        """
        ...
    @overload
    def nwave(self) -> int:
        """
        Get number of wavelengths for disort

        Returns:
          int: number of wavelengths for disort
        """
        ...
    @overload
    def nwave(self, nwave: int) -> DisortOptions:
        """
        Set number of wavelengths for disort

        Args:
          nwave (int): number of wavelengths for disort

        Returns:
          pydisort.DisortOptions: class object
        """
        ...
    @overload
    def ncol(self) -> int:
        """
        Get number of columns for disort

        Returns:
          int: number of columns for disort
        """
        ...
    @overload
    def ncol(self, ncol: int) -> DisortOptions:
        """
        Set number of columns for disort

        Args:
          ncol (int): number of columns for disort

        Returns:
          pydisort.DisortOptions: class object
        """
        ...
    @overload
    def accur(self) -> float:
        """
        Get accuracy for disort

        Returns:
          float: accuracy for disort
        """
        ...
    @overload
    def accur(self, accur: float) -> DisortOptions:
        """
        Set accuracy for disort

        Args:
          accur (float): accuracy for disort

        Returns:
          pydisort.DisortOptions: class object
        """
        ...
    @overload
    def upward(self) -> int:
        """
        Get direction for disort

        Returns:
          int: direction for disort
        """
        ...
    @overload
    def upward(self, upward: int) -> DisortOptions:
        """
        Set direction for disort

        Args:
          upward (int): direction for disort

        Returns:
          pydisort.DisortOptions: class object
        """
        ...
    @overload
    def user_tau(self) -> List[float]:
        """
        Get user optical depths for disort

        Returns:
          list[float]: user optical depths for disort
        """
        ...
    @overload
    def user_tau(self, user_tau: Union[List[float], ndarray]) -> DisortOptions:
        """
        Set user optical depths for disort

        Args:
          user_tau (list[float]): user optical depths for disort

        Returns:
          pydisort.DisortOptions: class object
        """
        ...
    @overload
    def user_mu(self) -> List[float]:
        """
        Get user zenith angles for disort

        Returns:
          list[float]: user zenith angles for disort
        """
        ...
    @overload
    def user_mu(self, user_mu: Union[List[float], ndarray]) -> DisortOptions:
        """
        Set user zenith angles for disort

        Args:
          user_mu (list[float]): user zenith angles for disort

        Returns:
          pydisort.DisortOptions: class object
        """
        ...
    @overload
    def user_phi(self) -> List[float]:
        """
        Get user azimuthal angles for disort

        Returns:
          list[float]: user azimuthal angles for disort
        """
        ...
    @overload
    def user_phi(self, user_phi: Union[List[float], ndarray]) -> DisortOptions:
        """
        Set user azimuthal angles for disort

        Args:
          user_phi (list[float]): user azimuthal angles for disort

        Returns:
          pydisort.DisortOptions: class object
        """
        ...
    @overload
    def wave_lower(self) -> List[float]:
        """
        Get lower wavenumber(length) at each bin for disort

        Returns:
          list[float]: lower wavenumber(length) at each bin for disort
        """
        ...
    @overload
    def wave_lower(
        self, wave_lower: Union[List[float], ndarray]
    ) -> DisortOptions:
        """
        Set lower wavenumber(length) at each bin for disort

        Args:
          wave_lower (list[float]): lower wavenumber(length) at each bin for disort

        Returns:
          pydisort.DisortOptions: class object
        """
        ...
    @overload
    def wave_upper(self) -> List[float]:
        """
        Get upper wavenumber(length) at each bin for disort

        Returns:
          list[float]: upper wavenumber(length) at each bin for disort
        """
        ...
    @overload
    def wave_upper(
        self, wave_upper: Union[List[float], ndarray]
    ) -> DisortOptions:
        """
        Set upper wavenumber(length) at each bin for disort

        Args:
          wave_upper (list[float]): upper wavenumber(length) at each bin for disort

        Returns:
          pydisort.DisortOptions: class object
        """
        ...
    @overload
    def ds(self) -> disort_state:
        """
        Get disort state for disort

        Returns:
          pydisort.disort_state: disort state for disort
        """
        ...
    @overload
    def ds(self, ds: disort_state) -> DisortOptions:
        """
        Set disort state for disort

        Args:
          ds (pydisort.disort_state): disort state for disort

        Returns:
          pydisort.DisortOptions: class object
        """
        ...

class Disort(nn.Module):
    """
    DISORT (Discrete Ordinates Radiative Transfer) module for radiative transfer calculations.

    This class wraps the DISORT radiative transfer solver and provides a PyTorch-compatible
    interface for computing radiative fluxes and intensities in plane-parallel atmospheres.
    """

    options: DisortOptions
    """Options used to configure this Disort instance"""

    @overload
    def __init__(self) -> None:
        """Construct a new default module."""
        ...
    @overload
    def __init__(self, options: DisortOptions) -> None:
        """
        Construct a Disort module

        Args:
          options (DisortOptions): Configuration options for DISORT
        """
        ...
    def __repr__(self) -> str: ...
    def forward(
        self,
        prop: torch.Tensor,
        bname: str = "",
        temf: Optional[torch.Tensor] = None,
        **kwargs: torch.Tensor,
    ) -> torch.Tensor:
        """
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
          **kwargs (Dict[str, torch.Tensor]): keyword arguments of disort boundary conditions, see keys listed above

        Returns:
          torch.Tensor: Radiative flux or intensity, shape (nwave, ncol, nlvl, nrad)

        Examples:
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
        """
        ...
    def release_cuda_workspace(self) -> None:
        """Release this instance's cached CUDA scratch workspace.

        CUDA forwards allocate and reuse a workspace sized for the largest
        batch seen by this instance. Call this method when that memory is no
        longer needed, for example before a large unrelated GPU workload.
        """
        ...
    def gather_flx(self) -> torch.Tensor:
        """
        Gather all disort flux outputs

        Returns:
          torch.Tensor: Disort flux outputs (nwave, ncol, nlvl = nlyr + 1, 8)

        Examples:
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
        """
        ...
    def gather_rad(self) -> torch.Tensor:
        """
        Gather all disort radiation outputs

        Returns:
          torch.Tensor: Disort radiation outputs (nwave, ncol, nlvl = nlyr + 1, 6)

        Examples:
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
        """
        ...

def scattering_moments(
    nmom: int, type: str, gg1: float = 0.0, gg2: float = 0.0, ff: float = 0.0
) -> torch.Tensor:
    """
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

      >>> import pydisort
      >>> pydisort.scattering_moments(4, 'isotropic')
      tensor([0., 0., 0., 0.], dtype=torch.float64)

      Example 2: Henyey-Greenstein phase function

      >>> import pydisort
      >>> pydisort.scattering_moments(4, 'henyey-greenstein', 0.85)
      tensor([0.8500, 0.7225, 0.6141, 0.5220], dtype=torch.float64)

      Example 3: Double Henyey-Greenstein phase function

      >>> import pydisort
      >>> pydisort.scattering_moments(4, 'double-henyey-greenstein', 0.85, 0.5, 0.5)
      tensor([0.6750, 0.4862, 0.3696, 0.2923], dtype=torch.float64)
    """
    ...
