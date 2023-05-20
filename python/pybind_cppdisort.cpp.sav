#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <cppdisort/cppdisort.h>

namespace py = pybind11;

PYBIND11_MODULE(pydisort, m) {
    py::class_<DisortWrapper>(m, "disort")
        //.def(py::init<>())
        .def_static("from_file", &DisortWrapper::FromFile, py::arg("filename"))
        .def("set_atmosphere_dimension", &DisortWrapper::SetAtmosphereDimension,
            py::arg("nlyr"), py::arg("nmom"), py::arg("nstr"), py::arg("nphase"))
        .def("set_flags", py::overload_cast<std::map<std::string, bool> const &>(&DisortWrapper::SetFlags),
            py::arg("flags"))
        .def("set_intensity_dimension", &DisortWrapper::SetIntensityDimension,
            py::arg("nphi"), py::arg("numu"), py::arg("ntau"))
        .def("finalize", &DisortWrapper::Finalize)
        .def("set_accuracy", &DisortWrapper::SetAccuracy, py::arg("accur"))
        .def("set_optical_depth", &DisortWrapper::SetOpticalDepth, py::arg("tau"), py::arg("len"))
        .def("set_single_scattering_albedo", &DisortWrapper::SetSingleScatteringAlbedo, py::arg("ssa"), py::arg("len"))
        .def("set_level_temperature", &DisortWrapper::SetLevelTemperature, py::arg("temp"), py::arg("len"))
        .def("set_wavenumber_range_invcm", &DisortWrapper::SetWavenumberRange_invcm, py::arg("wmin"), py::arg("wmax"))
        .def("set_wavenumber_invcm", &DisortWrapper::SetWavenumber_invcm, py::arg("wave"))
        .def("set_output_optical_depth", &DisortWrapper::SetOutputOpticalDepth, py::arg("usrtau"), py::arg("len"))
        .def("set_outgoing_ray", &DisortWrapper::SetOutgoingRay, py::arg("umu"), py::arg("phi"))
        //.def("set_planck_source", &DisortWrapper::SetPlanckSource, py::arg("planck"))
        //.def("set_legendre_coefficients", &DisortWrapper::SetLegendreCoefficients, py::arg("legendre"))
        .def("run_rt_flux", &DisortWrapper::RunRTFlux)
        .def("run_rt_intensity", &DisortWrapper::RunRTIntensity);

    py::register_exception<std::runtime_error>(m, "RuntimeError");
}
