// pybind11
#include <pybind11/pybind11.h>

// disort
#include <cdisort213/cdisort.h>

namespace py = pybind11;

void bind_cdisort(py::module &m) {
  py::class_<disort_state>(m, "disort_state")
      .def(py::init<>())
      .def_readwrite("nlyr", &disort_state::nlyr)
      .def_readwrite("nstr", &disort_state::nstr)
      .def_readwrite("nphase", &disort_state::nphase)
      .def_readwrite("nmom", &disort_state::nmom);
}
