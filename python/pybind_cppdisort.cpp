#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <cppdisort/cppdisort.h>

namespace py = pybind11;

// TODO(CLI): most function should support double, flat and int types

void setDisortArraysFromDict(DisortWrapper &disort, py::dict &kwargs) {
    // TODO(CLI): support more options
    if (kwargs.contains("temp")) {
        py::buffer_info info =
            py::cast<py::buffer>(kwargs["temp"]).request();
        if (info.format !=
                py::format_descriptor<double>::format() ||
            info.ndim != 1) {
            throw std::runtime_error("Incompatible buffer format!");
        }

        disort.SetLevelTemperature((double *)info.ptr,
                                   info.shape[0]);
    }

    if (kwargs.contains("ssa")) {
        py::buffer_info info =
            py::cast<py::buffer>(kwargs["ssa"]).request();
        if (info.format !=
                py::format_descriptor<double>::format() ||
            info.ndim != 1) {
            throw std::runtime_error("Incompatible buffer format!");
        }

        disort.SetSingleScatteringAlbedo((double *)info.ptr,
                                         info.shape[0]);
    }

    if (kwargs.contains("tau")) {
        py::buffer_info info =
            py::cast<py::buffer>(kwargs["tau"]).request();
        if (info.format !=
                py::format_descriptor<double>::format() ||
            info.ndim != 1) {
            throw std::runtime_error("Incompatible buffer format!");
        }
        disort.SetOpticalDepth((double *)info.ptr,
                                info.shape[0]);
    }

    if (kwargs.contains("pmom")) {
        py::buffer_info info =
            py::cast<py::buffer>(kwargs["pmom"]).request();
        if (info.format != py::format_descriptor<double>::format()) {
            throw std::runtime_error("Incompatible buffer format!");
        } else {
            if (info.ndim == 1) {
                disort.SetPhaseMoments((double *)info.ptr,
                                       1, info.shape[0]);
            } else if (info.ndim == 2) {
                disort.SetPhaseMoments((double *)info.ptr,
                        info.shape[0], info.shape[1]);
            } else {
                throw std::runtime_error("Incompatible buffer format!");
            }
        }
    }

    if (kwargs.contains("utau")) {
        py::buffer_info info =
            py::cast<py::buffer>(kwargs["utau"]).request();
        if (info.format !=
                py::format_descriptor<double>::format() ||
            info.ndim != 1) {
            throw std::runtime_error("Incompatible buffer format!");
        }
        disort.SetUserOpticalDepth((double *)info.ptr,
                info.shape[0]);
    }

    if (kwargs.contains("umu")) {
        py::buffer_info info =
            py::cast<py::buffer>(kwargs["umu"]).request();
        if (info.format !=
                py::format_descriptor<double>::format() ||
            info.ndim != 1) {
            throw std::runtime_error("Incompatible buffer format!");
        }
        disort.SetUserCosinePolarAngle((double *)info.ptr,
                info.shape[0]);
    }

    if (kwargs.contains("uphi")) {
        py::buffer_info info =
            py::cast<py::buffer>(kwargs["uphi"]).request();
        if (info.format !=
                py::format_descriptor<double>::format() ||
            info.ndim != 1) {
            throw std::runtime_error("Incompatible buffer format!");
        }
        disort.SetUserAzimuthalAngle((double *)info.ptr,
                info.shape[0]);
    }
}

PYBIND11_MODULE(pydisort, m) {
    m.def("get_legendre_coefficients", &getLegendreCoefficients,
        py::arg("nmom"), py::arg("model"), py::arg("gg") = 0.);

    py::class_<DisortWrapper>(m, "disort")
        // No need to expose the constructor, since we have static factory
        // methods such as from_{file,string,...}.
        .def_static("from_file",
            &DisortWrapper::FromFile)

        .def("set_header",
            &DisortWrapper::SetHeader
            )

        .def("set_atmosphere_dimension",
                &DisortWrapper::SetAtmosphereDimension,
            py::arg("nlyr"), py::arg("nmom"), py::arg("nstr"), py::arg("nphase")
            )
            
        .def("set_flags",
            py::overload_cast<std::map<std::string, bool> const &>(&DisortWrapper::SetFlags)
            )

        .def("set_intensity_dimension",
            &DisortWrapper::SetIntensityDimension,
            py::arg("nuphi"), py::arg("nutau"), py::arg("numu")
            )

        .def("finalize",
            &DisortWrapper::Finalize
            )

        .def("set_accuracy",
            &DisortWrapper::SetAccuracy
            )

        .def("set_optical_depth",
            [](DisortWrapper &disort, py::buffer b) {
                 py::buffer_info info = b.request();
                 if (info.format != py::format_descriptor<double>::format() ||
                     info.ndim != 1) {
                     throw std::runtime_error("Incompatible buffer format!");
                 }
                 // call the function
                 return disort.SetOpticalDepth((double *)info.ptr,
                                               info.shape[0]);
             })

        .def("set_optical_depth",
             [](DisortWrapper &disort, py::list lst) {
                 std::vector<double> optical_depth;
                 for (auto elem : lst) {
                     optical_depth.push_back(py::cast<double>(elem));
                 }
                 // call the function
                 return disort.SetOpticalDepth(optical_depth.data(),
                                               optical_depth.size());
             })

        .def("set_single_scattering_albedo",
             [](DisortWrapper &disort, py::buffer b) {
                 py::buffer_info info = b.request();
                 if (info.format != py::format_descriptor<double>::format() ||
                     info.ndim != 1) {
                     throw std::runtime_error("Incompatible buffer format!");
                 }
                 // call the function
                 return disort.SetSingleScatteringAlbedo((double *)info.ptr,
                                                         info.shape[0]);
             })

        .def("set_single_scattering_albedo",
             [](DisortWrapper &disort, py::list lst) {
                 std::vector<double> ssa;
                 for (auto elem : lst) {
                     ssa.push_back(py::cast<double>(elem));
                 }
                 // call the function
                 return disort.SetSingleScatteringAlbedo(ssa.data(),
                                                         ssa.size());
             })

        .def("set_level_temperature",
             [](DisortWrapper &disort, py::buffer b) {
                 py::buffer_info info = b.request();
                 if (info.format != py::format_descriptor<double>::format() ||
                     info.ndim != 1) {
                     throw std::runtime_error("Incompatible buffer format!");
                 }
                 // call the function
                 return disort.SetLevelTemperature((double *)info.ptr,
                                                   info.shape[0]);
             })

        .def("set_level_temperature",
             [](DisortWrapper &disort, py::list lst) {
                 std::vector<double> level_temperature;
                 for (auto elem : lst) {
                     level_temperature.push_back(py::cast<double>(elem));
                 }
                 // call the function
                 return disort.SetLevelTemperature(level_temperature.data(),
                                                   level_temperature.size());
             })

        .def("set_wavenumber_range_invcm",
            &DisortWrapper::SetWavenumberRange_invcm,
            py::arg("wmin"), py::arg("wmax")
            )

        .def("set_wavenumber_invcm",
            &DisortWrapper::SetWavenumber_invcm
            )

        .def("run", &DisortWrapper::Run)
        .def("run_with",
            [](DisortWrapper &disort, py::dict &kwargs) {
                setDisortArraysFromDict(disort, kwargs);
                return disort.Run();
            })
        .def("get_flux", &DisortWrapper::GetFlux)
        .def("get_intensity", &DisortWrapper::GetIntensity)

        .def("get_nmom", &DisortWrapper::nMoments)
        .def("get_nstr", &DisortWrapper::nStreams)
        .def("get_nlyr", &DisortWrapper::nLayers)

        .def_readwrite("btemp", &DisortWrapper::btemp)
        .def_readwrite("ttemp", &DisortWrapper::ttemp)
        .def_readwrite("fluor", &DisortWrapper::fluor)
        .def_readwrite("albedo", &DisortWrapper::albedo)
        .def_readwrite("fisot", &DisortWrapper::fisot)
        .def_readwrite("fbeam", &DisortWrapper::fbeam)
        .def_readwrite("temis", &DisortWrapper::temis)
        .def_readwrite("umu0", &DisortWrapper::umu0)
        .def_readwrite("phi0", &DisortWrapper::phi0);

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
