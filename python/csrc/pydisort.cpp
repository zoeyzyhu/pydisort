// torch
#include <torch/extension.h>

// fmt
#include <fmt/format.h>

// python
#include "pyoptions.hpp"

// disort
#include <disort/index.h>
#include <disort/disort.hpp>
#include <disort/disort_formatter.hpp>

namespace py = pybind11;

void bind_disort_options(py::module &m);
void bind_cdisort(py::module &m);

PYBIND11_MODULE(pydisort, m) {
  m.attr("__name__") = "pydisort";

  m.attr("kIRFLDIR") = 0;
  m.attr("kIFLDN") = 1;
  m.attr("kIFLUP") = 2;
  m.attr("kIDFDT") = 3;
  m.attr("kIUAVG") = 4;
  m.attr("kIUAVGDN") = 5;
  m.attr("kIUAVGUP") = 6;
  m.attr("kIUAVGSO") = 7;

  m.attr("kIEX") = (int)disort::PropertyIndex::IEX;
  m.attr("kISS") = (int)disort::PropertyIndex::ISS;
  m.attr("kIPM") = (int)disort::PropertyIndex::IPM;

  m.attr("kIUP") = (int)disort::DirectionIndex::IUP;
  m.attr("kIDN") = (int)disort::DirectionIndex::IDN;

  bind_cdisort(m);
  bind_disort_options(m);

  m.def("scattering_moments", &disort::scattering_moments, py::arg("nmom"),
        py::arg("type"), py::arg("gg1") = 0.0, py::arg("gg2") = 0.0,
        py::arg("ff") = 0.0);

  ADD_DISORT_MODULE(Disort, DisortOptions)
      .def_readonly("options", &disort::DisortImpl::options)
      .def("gather_flx", &disort::DisortImpl::gather_flx)
      .def("gather_rad", &disort::DisortImpl::gather_rad)
      .def("release_cuda_workspace", &disort::DisortImpl::release_cuda_workspace)
      .def(
          "forward",
          [](disort::DisortImpl &self, torch::Tensor prop, std::string bname,
             torch::optional<torch::Tensor> temf, const py::kwargs &kwargs) {
            // get bc from kwargs
            std::map<std::string, torch::Tensor> bc;
            for (auto item : kwargs) {
              auto key = py::cast<std::string>(item.first);
              auto value = py::cast<torch::Tensor>(item.second);
              bc.emplace(std::move(key), std::move(value));
            }

            for (auto &[key, value] : bc) {
              std::vector<std::string> items = {"fbeam", "albedo", "fluor",
                                                "fisot", "temis"};

              // broadcast dimensions to (nwave, ncol)
              if (std::find(items.begin(), items.end(), key) != items.end()) {
                while (value.dim() < 2) {
                  value = value.unsqueeze(0);
                }
              }
            }

            // broadcast dimensions to (nwave, ncol, nlyr, nprop)
            while (prop.dim() < 4) {
              prop = prop.unsqueeze(0);
            }

            return self.forward(prop, &bc, bname, temf);
          },
          py::arg("prop"), py::arg("bname") = "", py::arg("temf") = py::none());
}
