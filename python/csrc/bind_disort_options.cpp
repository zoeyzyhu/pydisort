// pybind11
#include <pybind11/stl.h>

// torch
#include <torch/extension.h>

// python
#include "pyoptions.hpp"

// disort
#include <disort/disort.hpp>
#include <disort/disort_formatter.hpp>

namespace py = pybind11;

void bind_disort_options(py::module &m) {
  auto pyDisortOptions =
      py::class_<disort::DisortOptionsImpl, disort::DisortOptions>(
          m, "DisortOptions");

  pyDisortOptions.def(py::init<>())
      .def("__repr__",
           [](const disort::DisortOptionsImpl &a) {
             std::stringstream ss;
             a.report(ss);
             return fmt::format("DisortOptions(\n{})", ss.str());
           })
      .ADD_OPTION(std::string, disort::DisortOptionsImpl, header)
      .ADD_OPTION(std::string, disort::DisortOptionsImpl, flags)
      .ADD_OPTION(int, disort::DisortOptionsImpl, nwave)
      .ADD_OPTION(int, disort::DisortOptionsImpl, ncol)
      .ADD_OPTION(double, disort::DisortOptionsImpl, accur)
      .ADD_OPTION(int, disort::DisortOptionsImpl, upward)
      .ADD_OPTION(std::vector<double>, disort::DisortOptionsImpl, user_tau)
      .ADD_OPTION(std::vector<double>, disort::DisortOptionsImpl, user_mu)
      .ADD_OPTION(std::vector<double>, disort::DisortOptionsImpl, user_phi)
      .ADD_OPTION(std::vector<double>, disort::DisortOptionsImpl, wave_lower)
      .ADD_OPTION(std::vector<double>, disort::DisortOptionsImpl, wave_upper)
      .ADD_OPTION(disort_state, disort::DisortOptionsImpl, ds);
}
