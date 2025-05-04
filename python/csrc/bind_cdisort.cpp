// pybind11
#include <pybind11/pybind11.h>

// disort
#include <disort/disort_formatter.hpp>

// cdisort
#include <cdisort213/cdisort.h>

namespace py = pybind11;

void bind_cdisort(py::module &m) {
  py::class_<disort_state>(m, "disort_state", R"(
This is a wrapper for the ``disort_state`` object in the C DISORT library.
The only important variables are:

- ``nlyr``: number of layers
- ``nstr``: number of streams
- ``nmom``: number of phase function moments
- ``nphase``: number of azimuthal angles

The result of the variables will be transferred from the :class:`pydisort.DisortOptions`
object when the :class:`pydisort.Disort <disort.cpp.Disort>` object is created.
      )")

      .def(py::init<>(), R"(
Create a new default DISORT state object.

Returns:
  pydisort.disort_state: class object

Examples:
.. code-block::

  >>> import pydisort
  >>> ds = pydisort.disort_state()
  >>> ds.nlyr, ds.nstr, ds.nphase = 10, 4, 4
  >>> print(ds)
  disort_state(nlyr = 10; nstr = 4; nmom = 0; ibcnd = 0; usrtau = 0; usrang = 0; lamber = 0; planck = 0; spher = 0; onlyfl = 0)
          )")

      .def("__repr__",
           [](const disort_state &a) {
             return fmt::format("disort_state{}", a);
           })

      .def_readwrite("nlyr", &disort_state::nlyr, R"(Number of layers)")

      .def_readwrite("nstr", &disort_state::nstr, R"(Number of streams)")

      .def_readwrite("nphase", &disort_state::nphase,
                     R"(Number of azimuthal angles)")

      .def_readwrite("nmom", &disort_state::nmom,
                     R"(Number of phase functions moments)");
}
