User Guide
==========

This page explains the pieces every pydisort program is built from: how a run
is configured, what shape the inputs and outputs have, and how the degenerate
dimensions are broadcast. If you just want to get something running, start with
:doc:`installation`; for complete worked calculations, see :doc:`examples`.

How a run is configured
-----------------------

The normal usage of pydisort is to create a :class:`pydisort.DisortOptions`
object first and then initialize the :class:`pydisort.cpp.Disort` object with
the :class:`pydisort.DisortOptions` object by:

.. code-block:: python

  >>> import pydisort
  >>> op = pydisort.DisortOptions().flags("onlyfl,lamber")
  >>> op.ds().nlyr = 4
  >>> op.ds().nstr = 4
  >>> op.ds().nmom = 4
  >>> op.ds().nphase = 4
  >>> ds = pydisort.Disort(op)

.. note::

  You can print the :class:`pydisort.DisortOptions` object to see its current settings:

  .. code-block:: python

    >>> print(op)
    DisortOptions(flags = onlyfl,lamber; nwave = 1; ncol = 1; wave = (); disort_state = (nlyr = 539784046; nstr = 1701994784; nmom = 2036689012; ibcnd = 0; usrtau = 0; usrang = 0; lamber = 0; planck = 0; spher = 0; onlyfl = 0))

  Note that the numbers in `disort_state` are not meaningful now because disort has not been properly initialized yet.
  Initializing of the disort state is done when the :class:`pydisort.cpp.Disort` object is created
  from the :class:`pydisort.DisortOptions` object:

  .. code-block:: python

    >>> ds = pydisort.Disort(op)
    >>> print(ds.options)
    DisortOptions(flags = onlyfl,lamber; nwave = 1; ncol = 1; wave = (); disort_state = (nlyr = 4; nstr = 4; nmom = 4; ibcnd = 0; usrtau = 0; usrang = 0; lamber = 1; planck = 0; spher = 0; onlyfl = 1))

Understanding the dimensions
----------------------------

Example 1: Calculate attenuation of radiative flux in a plane-parallel atmosphere

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

It is important to understand the dimensions of the input and output arrays.
The input array `tau` has two dimensions. In order of appearance, they are:

  #. The layer dimension (nlyr = 4),
  #. The property dimension (nprop = 1).

Since this problem only has optical thickness, the property dimension is 1.
In the general case the property dimension holds, in order, the optical
thickness, the single-scattering albedo, and then ``nmom`` phase-function
moments, so ``nprop = 2 + nmom``.
If not specified, both the wavelength/wavenumber dimension and the column dimension
are assumed to be 1 and are automatically added internally to the input array.

The boundary condition for the problem such as the beam illuminance is provided as the keyword argument of the `forward` method.
The dimensions are automatically broadcasted to account for the degenerate wavelength/wavenumber and column dimensions:

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

The two batch dimensions
------------------------

``nwave`` and ``ncol`` are the dimensions pydisort parallelizes over, and they
are the reason it is faster than a single-threaded C implementation on a
multi-core machine. Plane-parallel radiative transfer treats each
(wavelength, column) pair independently, so the work spreads across threads
with no communication.

In practice this means: **batch your problem instead of looping over it.**
Filling a ``(nwave, ncol, nlyr, nprop)`` tensor and making one ``forward``
call is much faster than making ``nwave * ncol`` separate calls.

:doc:`examples` shows both axes in use. Example 2 batches eight spectral
bands, while Examples 3 and 4 batch independent atmospheric cases along the
column axis. :doc:`benchmarks` quantifies the resulting speed-up.

Thermal emission
----------------

Example 2: Calculate thermal emission of a medium with a temperature profile

.. code-block:: python

  >>> import torch
  >>> from pydisort import DisortOptions, Disort
  >>> op = DisortOptions().flags("onlyfl,lamber,planck")
  >>> op.ds().nlyr = 4
  >>> op.ds().nstr = 4
  >>> op.ds().nmom = 4
  >>> op.ds().nphase = 4
  >>> op.nwave(1)
  >>> op.wave_lower([20.])
  >>> op.wave_upper([4000.])
  >>> ds = Disort(op)
  >>> tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).unsqueeze(-1)
  >>> ds.forward(tau,
                 temf=torch.linspace(200, 240, 5).unsqueeze(0),
                 albedo=torch.tensor([0.]),
                 btemp=torch.tensor([240.]),
                 ttemp=torch.tensor([0.]),
                 temis=torch.tensor([1.]),
                 fisot=torch.tensor([0.]),
                 )
  tensor([[[[145.2179,   0.0000],
        [153.2714,  17.5241],
        [165.8178,  47.9667],
        [179.6727,  84.7606],
        [188.1117, 124.8982]]]])

Calculating thermal emission requires a temperature profile (``temf``)
and various boundary conditions such as surface albedo (``albedo``),
bottom temperature (``btemp``), top temperature (``ttemp``), etc.
You can pass those in as keyword arguments to the `forward` method, or organize them in a dictionary such as

.. code-block:: python

  >>> bc = {
  >>>   "albedo": torch.tensor([0.]),
  >>>   "btemp": torch.tensor([240.]),
  >>>   "ttemp": torch.tensor([0.]),
  >>>   "temis": torch.tensor([1.]),
  >>>   "fisot": torch.tensor([0.]),
  >>> }
  >>> ds.forward(tau, temf=torch.linspace(200, 240, 5).unsqueeze(0), **bc)
  tensor([[[[145.2179,   0.0000],
        [153.2714,  17.5241],
        [165.8178,  47.9667],
        [179.6727,  84.7606],
        [188.1117, 124.8982]]]])

The band limits given to ``wave_lower`` and ``wave_upper`` matter: cdisort
integrates the Planck function over each band, so the band must be wide enough
to capture the emission at the temperatures involved. A band of
20-4000 cm\ :sup:`-1` recovers :math:`\sigma T^4` to better than
10\ :sup:`-5` at terrestrial temperatures; hotter atmospheres need a wider
upper limit.

Getting more than the flux
--------------------------

:meth:`~pydisort.Disort.forward` returns upward and downward flux. Two further
accessors expose the rest of the solver output:

* :meth:`~pydisort.Disort.gather_flx` returns all eight cdisort flux fields,
  shape ``(nwave, ncol, nlvl, 8)``. Index them with the module constants
  ``pydisort.kIRFLDIR`` (direct beam), ``kIFLDN`` (diffuse downward),
  ``kIFLUP`` (upward), ``kIDFDT`` (flux divergence), and the ``kIUAVG*``
  mean-intensity fields.
* :meth:`~pydisort.Disort.gather_rad` returns radiances at the user-specified
  angles, shape ``(nwave, ncol, nphi, ntau, numu)``. It requires the ``usrang``
  flag.

Example 1 in :doc:`examples` uses ``gather_flx`` to isolate the direct beam;
Example 3 uses ``gather_rad`` to build a remote-sensing lookup table.

.. _troubleshooting:

Troubleshooting
---------------
- The most common error is "RuntimeError: DisortImpl::forward", which indicates
  that the disort run has failed. This error is mostly due to incorrect input
  dimensions or values. The error message shall provide more information on the
  cause of the error.

- Make sure that you have `lamber` in your flags, otherwise DISORT will panic and
  emit the following error:

   .. code-block:: text

      bidir_reflectivity--surface BDRF model .... not known
        ******* ERROR >>>>>> Existing...

- ``Input variable ds.utau in error`` means a value passed to ``user_tau``
  exceeds the total optical depth of the column. Note that ``user_tau`` is
  shared by every column in a batch, so it must not exceed the total optical
  depth of the *thinnest* column; ``0.0`` is always safe.

- ``Input variable ds.temper in error`` means a temperature in ``temf`` is
  outside the range cdisort accepts, most often a negative or zero value
  produced by an unstable outer iteration.

- The program should not exit unexpectedly. If the program exits unexpectedly,
  please report the issue to the author (zoey.zyhu@gmail.com).

.. tip::

  - Number of atmosphere levels is one more than the number of atmosphere layers.

  - Temperature is defined on levels, not layers. Other properties such as
    optical thickness, single scattering albedo, and moments of scattering phase function
    are defined on layers.

  - You can use ``print()`` method to print some of the DISORT internal states.

  - If you want to have more insights into DISORT internal inputs,
    you can set the ``print-input`` flag to ``True``.
    The DISORT internal inputs will be printed to the standard output
    when the ``forward()`` method is called.

  - You can use ``torch.set_default_dtype(torch.float64)`` to set the default
    data type to double precision.

The underlying DISORT algorithm is described by Stamnes et al. (1988) and its
C implementation by Buras & Dowling (1996); both are cited on the
:doc:`index` page.
