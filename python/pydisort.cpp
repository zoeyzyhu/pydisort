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
  >>> op = DisortOptions().header("running disort test").flags("onlyfl")
  >>> op.ds().nlyr = 4
  >>> op.ds().nstr = 4
  >>> op.ds().nmom = 4
  >>> ds = Disort(op)
  >>> tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).reshape((1,1,4,1))
  >>> bc = {"fbeam" : torch.tensor([3.14159]).reshape((1,1))}
  >>> result = ds.forward(tau, bc, "", None)
  >>> flx
  array([[3.14159   , 0.        , 0.        ],
         [2.84262818, 0.        , 0.        ],
         [2.32734711, 0.        , 0.        ],
         [1.72414115, 0.        , 0.        ],
         [1.15572637, 0.        , 0.        ]])

  In the example above, flx has two dimensions. The first dimension is number of atmosphere
  levels (nlyr + 1 = 5), and the second dimension is number of extracted flux fields (3).
  ``RFDLIR``, ``FLDN``, and ``FLUP`` are the indices representing the three flux fields:
  direct flux, diffuse flux, and upward flux, respectively.
  Consult ``pydisort.run()`` method for more information on the indices of flux fields.
  The attenuation of radiative flux is according to the Beer-Lambert law, i.e.,

  .. math::

    F(z) = F(0) \exp(-\tau(z)),

  where :math:`F(z)` is the radiative flux at level :math:`z`,
  :math:`F(0)` is the radiative flux at the top of the atmosphere, and :math:`\tau(z)` is the
  optical depth from the top of the atmosphere to level :math:`z`. The default direction of
  radiative flux is nadir.

  - Example 2: Calculate intensity from isotropic scattering in a plane-parallel atmosphere

  >>> import torch
  >>> from pydisort import DisortOptions, Disort
  >>> op = DisortOptions().flags("usrtau,usrang").output_rad(True)
  >>> op.ds().nlyr(1).nstr(16).nmom(16)
  >>> op.user_tau([0.0, 0.1]).user_mu([-1.0, -0.5, -0.1, 0.1, 0.5, 1.0]).user_phi([0.0])
  >>> ds = Disort(op)
  >>> prop = torch.tensor([[[0.1, 1.0]]], dtype=torch.float64)
  >>> ftoa = torch.tensor([[3.14159]], dtype=torch.float64)
  >>> bc = torch.zeros((op.nwave(), op.ncol(), 5), dtype=torch.float64)
  >>> bc[:, :, iumu0] = 0.1
  >>> rad = ds.forward(prop, ftoa, bc)
  >>> rad
  array([[[0.        , 0.        , 0.        , 0.18095504, 0.0516168 , 0.02707849],
          [0.02703935, 0.05146774, 0.17839685, 0.        , 0.        , 0.        ]]])

  The intensity array ``rad`` has three dimensions. The first dimension is the
  azimuthal angles (1). The second dimension is optical depth (2).
  The third dimension is the polar angles (6). The result is interpreted as backsattering
  at the top of the atmosphere (optical depth = 0.0) and forward scattering at the bottom
  of the atmosphere (optical depth = 0.1).


  Troubleshooting
  ---------------
  - The most common error is "RuntimeError: DisortWrapper::Run failed". When
    this error occurs, please check the error message printed before the
    error message. The error message printed before the error message
    usually provides more information on the cause of the error. Once you identify
    the cause of the error, you can fix the error by calling ``unseal()`` method,
    then setting the correct values, and then calling ``seal()`` method again.

  - One common issue that results in "RuntimeError: DisortWrapper::Run failed"
    is incompatible flags for flux or intensity calculations. For example, ``usrtau``
    and ``usrang`` flags should set to ``False`` when ``onlyfl`` flag is set to ``True``.

  - Another common issue is setting the wrong values for temperature, optical thickness,
    single scattering albedo, or phase function moments. All these values must be
    positive. If you set a negative value, you will get "RuntimeError: DisortWrapper::Run failed".

  - The program should not exit unexpectedly. If the program exits unexpectedly,
    please report the issue to the author (zoey.zyhu@gmail.com).

  Important Dimensions
  --------------------
  nlyr
      number of atmosphere layers
  nstr
      number of radiation streams
  nmom
      number of phase function moments in addition to the zeroth moment

  Tips
  ----
  - Number of atmosphere levels is one more than the number of atmosphere layers.

  - Temperature is defined on levels, not layers. Other properties such as
    optical thickness, single scattering albedo, and phase function moments
    are defined on layers.

  - You can use ``print()`` method to print some of the DISORT internal states.

  - You can chain methods such as ``set_flags``, ``set_atmosphere_dimension()``,
    ``set_intensity_dimension()``, and ``seal()`` together.

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

  ADD_DISORT_MODULE(Disort, DisortOptions);

  bind_disort_options(m);
  bind_phase_options(m);
  bind_cdisort(m);
}
