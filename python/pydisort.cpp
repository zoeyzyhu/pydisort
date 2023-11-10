// pybind11
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

// pydisort
#include <cppdisort/cppdisort.hpp>

namespace py = pybind11;

PYBIND11_MODULE(pydisort, m) {
  m.def("get_legendre_coefficients", &getLegendreCoefficients, py::arg("nmom"),
        py::arg("model"), py::arg("gg") = 0.);

  py::class_<DisortWrapper>(m, "disort")
      .def_readwrite("btemp", &DisortWrapper::btemp)
      .def_readwrite("ttemp", &DisortWrapper::ttemp)
      .def_readwrite("fluor", &DisortWrapper::fluor)
      .def_readwrite("albedo", &DisortWrapper::albedo)
      .def_readwrite("fisot", &DisortWrapper::fisot)
      .def_readwrite("fbeam", &DisortWrapper::fbeam)
      .def_readwrite("temis", &DisortWrapper::temis)
      .def_readwrite("umu0", &DisortWrapper::umu0)
      .def_readwrite("phi0", &DisortWrapper::phi0)

      // No need to expose the constructor, since we have static factory
      // methods such as from_{file,string,...}.
      .def_static("from_file", &DisortWrapper::FromFile)

      .def(py::init())
      .def(py::init([](py::dict &kwargs) {
        DisortWrapper disort;
        std::map<std::string, bool> dict;

        for (auto item : kwargs) {
          std::string key = item.first.cast<std::string>();
          int value = item.second.cast<bool>();
          dict[key] = value;
        }

        disort.SetFlags(dict);
        return disort;
      }))

      .def("set_header", &DisortWrapper::SetHeader)

      .def("__str__", &DisortWrapper::ToString)

      .def("set_accuracy", &DisortWrapper::SetAccuracy)
      .def("set_flags", &DisortWrapper::SetFlags)

      .def("set_atmosphere_dimension", &DisortWrapper::SetAtmosphereDimension,
           py::arg("nlyr"), py::arg("nmom"), py::arg("nstr"), py::arg("nphase"))
      .def("set_intensity_dimension", &DisortWrapper::SetIntensityDimension,
           py::arg("nuphi"), py::arg("nutau"), py::arg("numu"))

      .def("seal", &DisortWrapper::Seal)
      .def("unseal", &DisortWrapper::Unseal)

      .def("set_user_optical_depth", &DisortWrapper::SetUserOpticalDepth)
      .def("set_user_cosine_polar_angle", &DisortWrapper::SetUserCosinePolarAngle)
      .def("set_user_azimuthal_angle", &DisortWrapper::SetUserAzimuthalAngle)

      .def("set_wavenumber_invcm", &DisortWrapper::SetWavenumber_invcm)
      .def("set_wavenumber_range_invcm",
           &DisortWrapper::SetWavenumberRange_invcm, py::arg("wmin"),
           py::arg("wmax"))

      .def("set_optical_thickness", &DisortWrapper::SetOpticalThickness)
      .def("set_single_scattering_albedo", &DisortWrapper::SetSingleScatteringAlbedo)
      .def("set_temperature_on_level", &DisortWrapper::SetTemperatureOnLevel)
      .def("set_phase_moments", 
          [](DisortWrapper &disort, py::array_t<double> &pmom) {
            py::buffer_info info = pmom.request();
            if (info.format != py::format_descriptor<double>::format()) {
              throw std::runtime_error("Incompatible buffer format!");
            } else {
              if (info.ndim == 1) {
                disort.SetPhaseMoments(static_cast<double *>(info.ptr), 1,
                                       info.shape[0]);
              } else if (info.ndim == 2) {
                disort.SetPhaseMoments(static_cast<double *>(info.ptr),
                                       info.shape[0], info.shape[1]);
              } else {
                throw std::runtime_error("Incompatible buffer format!");
              }
            }
          })

      .def("run", &DisortWrapper::Run)

      //! \todo better api fro getting fluxes
      .def("get_flux",
          [](DisortWrapper &disort) {
            py::array_t<double> ndarray({disort.nLayers() + 1, 8}, &disort.Result()->rad[0].rfldir);
            return ndarray;
          })
      .def("get_intensity",
          [](DisortWrapper &disort) {
            py::array_t<double> ndarray({disort.nUserAzimuthalAngles(), 
                                         disort.nUserOpticalDepths(),
                                         disort.nUserPolarAngles()}, disort.Result()->uu);
            return ndarray;
          })

      .def("get_nmom", &DisortWrapper::nMoments)
      .def("get_nstr", &DisortWrapper::nStreams)
      .def("get_nlyr", &DisortWrapper::nLayers);

  //! \todo will be removed
  py::class_<Radiant>(m, "Radiant")
      .def(py::init<>())
      .def_readonly_static("RFLDIR", &Radiant::RFLDIR)
      .def_readonly_static("FLUP", &Radiant::FLUP)
      .def_readonly_static("FLDN", &Radiant::FLDN)
      .def_readonly_static("DFDT", &Radiant::DFDT)
      .def_readonly_static("UAVG", &Radiant::UAVG)
      .def_readonly_static("UAVGDN", &Radiant::UAVGDN)
      .def_readonly_static("UAVGUP", &Radiant::UAVGUP)
      .def_readonly_static("UAVGSO", &Radiant::UAVGSO);
}
