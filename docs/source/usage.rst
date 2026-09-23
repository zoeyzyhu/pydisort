User Guide
==========

This page explains the pieces every pydisort program is built from: how a run
is configured, what shape the inputs and outputs have, and which singleton
dimensions can be omitted. If you just want to get something running, start with
:doc:`installation`; for complete worked calculations, see :doc:`examples`.

How a run is configured
-----------------------

The normal usage of pydisort is to create a :class:`pydisort.DisortOptions`
object first and then initialize the :class:`pydisort.Disort` object with
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

   Inspect dimensions through the options' state rather than relying on the
   formatting of the printed representation:

   .. code-block:: python

      >>> op.ds().nlyr, op.ds().nstr, op.ds().nmom
      (4, 4, 4)

   Configure options before constructing the solver. Construction allocates
   its internal arrays. Do not change dimensions or flags afterwards;
   construct a new solver for a new configuration.

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
The binding inserts missing leading singleton dimensions: ``(nlyr, nprop)``
becomes ``(1, 1, nlyr, nprop)``, and ``(ncol, nlyr, nprop)`` becomes
``(1, ncol, nlyr, nprop)``. It does not replicate optical properties across
larger batch dimensions. The resulting shape must match the configured
``nwave``, ``ncol`` and ``nlyr``.

Boundary conditions are keyword arguments of ``forward``. For the unprefixed
spectral keys ``fbeam``, ``albedo``, ``fluor``, ``fisot`` and ``temis``, the
binding similarly inserts leading singleton dimensions until the input is 2D.
This is dimension insertion, not general PyTorch broadcasting: a one-element
beam tensor becomes ``(1, 1)`` and is rejected for a two-column problem.

In the example above, flx has four dimensions. In order of appearance, they are:

  #. The wavelength/wavenumber dimension (nwave = 1),
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

.. list-table:: Input shapes after singleton insertion
   :header-rows: 1
   :widths: 40 30 30

   * - Input
     - Required shape
     - Shared across wavelengths?
   * - ``prop``
     - ``(nwave, ncol, nlyr, nprop)``
     - No; fill or expand explicitly.
   * - ``fbeam``, ``albedo``, ``fluor``, ``fisot``, ``temis``
     - ``(nwave, ncol)``
     - No; fill or expand explicitly.
   * - ``umu0``, ``phi0``, ``btemp``, ``ttemp``
     - ``(ncol,)``
     - Yes, by the solver's dispatch layer.
   * - ``temf``
     - ``(ncol, nlyr + 1)``
     - Yes, by the solver's dispatch layer.

For example, give every wave/column the same incident beam by constructing
the full tensor. Geometry remains one value per column:

.. testcode:: batch-shapes

   import torch
   from pydisort import Disort, DisortOptions

   torch.set_default_dtype(torch.float64)
   op = DisortOptions().flags("onlyfl,lamber,quiet").nwave(2).ncol(2)
   op.ds().nlyr = 4
   op.ds().nstr = op.ds().nmom = op.ds().nphase = 4
   solver = Disort(op)
   prop = torch.full((2, 2, 4, 1), 0.1)
   flux = solver.forward(
       prop, fbeam=torch.full((2, 2), 3.14159), umu0=torch.ones(2)
   )
   assert flux.shape == (2, 2, 5, 2)

Alternatively, use ``value.expand(nwave, ncol).contiguous()`` to repeat an
existing scalar or compatible tensor explicitly. With a nonempty ``bname``,
pass matching prefixed keys such as ``"B1/fbeam"``; these currently require
the full 2D spectral shape even for singleton batches. ``btemp`` and ``ttemp``
are never prefixed. Omitted boundary inputs use solver defaults, rather than
being inferred from another supplied boundary tensor.

.. _python-flag-support:

Flag availability in Python
---------------------------

Use ``lamber`` for the lower boundary, ``onlyfl`` for flux-only calculations,
``planck`` for thermal emission, ``usrtau`` for requested output depths and
``usrang`` for requested viewing directions. The API reference also lists
intensity-correction and diagnostic flags.

Recognizing a flag name is not the same as exposing a complete feature:

* ``ibcnd`` is recognized, but ``forward`` rejects the special-boundary mode.
* ``spher`` requires the body's radius and level altitudes; those inputs are
  not exposed by the public Python interface. Do not enable it.
* ``general_source`` requires user-source arrays that are not exposed by the
  public Python interface. Do not enable it.
* ``output_uum`` requests Fourier components for which there is no public
  Python output accessor; it is not a supported output workflow.

These are backend capabilities, not supported Python features.

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

The limits passed to ``wave_lower`` and ``wave_upper`` are wavenumbers in
cm\ :sup:`-1`. cdisort integrates the Planck function over each finite band;
:math:`\sigma T^4` is the blackbody flux integrated over the entire spectrum.
For a 20-4000 cm\ :sup:`-1` band, the omitted fraction of that total is about
:math:`5.23\times10^{-5}` at 288 K and :math:`1.45\times10^{-4}` at 200 K.
These are spectral truncation errors, not solver errors. Choose both band
endpoints for the temperature range and accuracy required, or compare against
an independently integrated Planck flux over exactly the same bands.

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
C implementation by Buras, Dowling & Emde (2011); both are cited on the
:doc:`index` page.
