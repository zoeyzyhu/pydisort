// torch
#include <torch/extension.h>

// python
#include "pyoptions.hpp"

// disort
#include <disort/disort.hpp>
#include <disort/disort_formatter.hpp>

namespace py = pybind11;

void bind_disort_options(py::module &m) {
  auto pyDisortOptions = py::class_<disort::DisortOptions>(m, "DisortOptions");

  pyDisortOptions
      .def(py::init<>())

      .def("__repr__",
           [](const disort::DisortOptions &a) {
             std::stringstream ss;
             a.report(ss);
             return fmt::format("DisortOptions(\n{})", ss.str());
           })

      .ADD_OPTION(std::string, disort::DisortOptions, header)

      .ADD_OPTION(std::string, disort::DisortOptions, flags)

      .ADD_OPTION(int, disort::DisortOptions, nwave)

      .ADD_OPTION(int, disort::DisortOptions, ncol)

      .ADD_OPTION(double, disort::DisortOptions, accur)

      .ADD_OPTION(int, disort::DisortOptions, upward)

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, user_tau)

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, user_mu)

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, user_phi)

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, wave_lower)

      .ADD_OPTION(std::vector<double>, disort::DisortOptions, wave_upper)

      .ADD_OPTION(disort_state, disort::DisortOptions, ds);
}
