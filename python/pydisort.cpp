// pybind11
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

// pydisort
#include <cppdisort/cppdisort.hpp>

namespace py = pybind11;

PYBIND11_MODULE(pydisort, m) {
  m.doc() = "Python bindings for disort.";

  m.attr("RFLDIR") = 0;
  m.attr("FLDN") = 1;
  m.attr("FLUP") = 2;
  m.attr("DFDT") = 3;
  m.attr("UAVG") = 4;
  m.attr("UAVGDN") = 5;
  m.attr("UAVGUP") = 6;
  m.attr("UAVGSO") = 7;

  m.def("get_phase_function", &get_phase_function, py::arg("nmom"),
        py::arg("model"), py::arg("gg") = 0.);

  py::class_<DisortWrapper>(m, "disort")
      .def_readwrite("btemp", &DisortWrapper::btemp, R"(
          Bottom boundary temperature (K)
          )")
      .def_readwrite("ttemp", &DisortWrapper::ttemp, R"(
          Top boundary temperature (K)
          )")
      .def_readwrite("fluor", &DisortWrapper::fluor, R"(
          Intensity of bottom-boundary isotropic illumination
          )")
      .def_readwrite("albedo", &DisortWrapper::albedo, R"(
          Surface albedo. Needed if lamber = True
          )")
      .def_readwrite("fisot", &DisortWrapper::fisot, R"( 
          Intensity of top-boundary isotropic illumination
          )")
      .def_readwrite("fbeam", &DisortWrapper::fbeam, R"(
          Intensity of top-boundary incident parallel beam illumination
          )")
      .def_readwrite("temis", &DisortWrapper::temis, R"(
          Emissivity of top boundary. Needed if lamber = True
          )")
      .def_readwrite("umu0", &DisortWrapper::umu0, R"( 
          Cosine of incident beam zenith angle (positive)
          )")
      .def_readwrite("phi0", &DisortWrapper::phi0, R"( 
          Azimuthal angle of incident beam
          )")

      .def(py::init())

      .def("set_header", &DisortWrapper::SetHeader, R"(
          Set header for disort output

          Parameters
          ----------
          arg0 : str
              Header string

          Returns
          -------
          None
          )")

      .def("__str__", &DisortWrapper::ToString)

      .def("set_accuracy", &DisortWrapper::SetAccuracy, R"(
          Set accuracy of disort convergence

          Parameters
          ----------
          arg0 : float
              Accuracy of disort convergence. Default value is 1e-6

          Returns
          -------
          None
          )")

      .def("set_flags", &DisortWrapper::SetFlags, R"( 
          Set radiation flags for disort

          Parameters
          ----------
          arg0 : dict
            Dictionary of radiation flags. The following flags are supported:
            'ibcnd': [True, False], defaults to False
                False: General boundary conditions that allows:
                * beam illumination from the top (set fbeam)
                * isotropic illumination from the top (set fisot)
                * thermal emission from the top (set ttemp and temis)
                * interal thermal emission (use set_temperature_on_level)
                * reflection at the bottom (set lamber, albedo)
                * thermal emission from the bottom (set btemp)
                True: Special boundary condition that only returns only albedo and
                transmissivity of the entire medium.
                * pydisort has limited support for this option. Consult the
                    documentation of DISORT for more details.
            'usrtau': [True, False], defaults to True
                use user optical depths
            'usrang': [True, False], defaults to True
                use user azimuthal angles
            'lamber': [True, False], defaults to True
                turn on lambertian reflection surface
            'plank': [True, False], defaults to True
                turn on plank source (thermal emission)
            'spher': [True, False], defaults to False
                turn on spherical correction
            'onlyfl': [True, False], defaults to False
                only compute radiative fluxes
            'quiet': [True, False], defaults to True
                turn on disort internal printout
            'intensity_correction': [True, False], defaults to True
                turn on intensity correction
            'old_intensity_correction': [True, False], defaults to True
                turn on old intensity correction
            'general_source': [True, False], defaults to False
                turn on general source
            'output_uum': [True, False], defaults to False
                output azimuthal components of the intensity
            'print-input': [True, False], defaults to False
                print input parameters
            'print-fluxes': [True, False], defaults to False
                print fluxes
            'print-intensity': [True, False], defaults to False
                print intensity
            'print-transmissivity': [True, False], defaults to False
                print transmissivity
            'print-phase-function': [True, False], defaults to False
                print phase function

          Returns
          -------
          DisortWrapper object
          )")

      .def("set_atmosphere_dimension", &DisortWrapper::SetAtmosphereDimension, R"(
          Set atmosphere dimension for disort

          Parameters
          ----------
          nlyr : int
              Number of layers
          nmom : int
              Number of phase function moments
          nstr : int
              Number of streams (computational polar angles)
          nphase : int
              Number of angles (grid points)

          Returns
          -------
          DisortWrapper object

          Example
          -------
          >>> import pydisort
          >>> disort = pydisort.disort()
          >>> disort.set_atmosphere_dimension(1, 4, 4, 4)
          >>> print(disort)

          Notes
          -----
          It is common to set nstr = nphase = nmom.
          Memory was not allocated until seal() is called.
          )",
           py::arg("nlyr"), py::arg("nmom"), py::arg("nstr"), py::arg("nphase"))

      .def("set_intensity_dimension", &DisortWrapper::SetIntensityDimension, R"(
          Set intensity dimension for disort

          Parameters
          ----------
          nuphi : int
              Number of user azimuthal angles
          nutau: int
              Number of user optical depths
          numu : int
              Number of user polar angles

          Returns
          -------
          DisortWrapper object

          Example
          -------
          >>> import pydisort
          >>> disort = pydisort.disort()
          >>> disort.set_intensity_dimension(1, 4, 4)

          Notes
          -----
          This function should be called when intensity calculations are requested.
          Memory was not allocated until seal() is called.
          )",
           py::arg("nuphi"), py::arg("nutau"), py::arg("numu"))

      .def("seal", &DisortWrapper::Seal, R"(
          Seal disort object after setting the dimensions

          Returns
          -------
          None

          Notes
          -----
          Internal memory is allocated after seal() is called.
          )")

      .def("unseal", &DisortWrapper::Unseal, R"(
          Unseal disort object to allow resetting the dimensions

          Returns
          -------
          None

          Notes
          -----
          Internal memory is deallocated after unseal() is called.
          )")

      .def("set_user_optical_depth", &DisortWrapper::SetUserOpticalDepth, R"(
          Set user optical depth for disort

          Parameters
          ----------
          tau : array_like
              User optical depth

          Returns
          -------
          None
          )")

      .def("set_user_cosine_polar_angle", &DisortWrapper::SetUserCosinePolarAngle, R"(
          Set user cosine polar angle for disort

          Parameters
          ----------
          arg0 : array_like
              User cosine polar angle

          Returns
          -------
          None

          Notes 
          -----
          Must be called after seal()
          )")

      .def("set_user_azimuthal_angle", &DisortWrapper::SetUserAzimuthalAngle, R"(
          Set user azimuthal angle for disort

          Parameters
          ----------
          arg0 : array of floats
              User azimuthal angle

          Returns
          -------
          None

          Notes 
          -----
          Must be called after seal()
          )")

      .def("set_wavenumber_invcm", &DisortWrapper::SetWavenumber_invcm, R"(
          Set a wavenumber for disort

          Parameters
          ----------
          arg0 : float
              Wavenumber

          Returns
          -------
          None
          )")

      .def("set_wavenumber_range_invcm", &DisortWrapper::SetWavenumberRange_invcm,
          R"(
          Set a wavenumber range for disort

          Parameters
          ----------
          wmin : float
              Minimum wavenumber
          wmax : float
              Maximum wavenumber

          Returns
          -------
          None
          )",
           py::arg("wmin"), py::arg("wmax"))

      .def("set_optical_thickness", &DisortWrapper::SetOpticalThickness,
          R"(
          Set layer optical thickness

          Parameters
          ----------
          arg0 : array of floats
              Optical thickness

          Returns
          -------
          None
          )")

      .def("set_single_scattering_albedo", &DisortWrapper::SetSingleScatteringAlbedo,
          R"(
          Set layer single scattering albedo

          Parameters
          ----------
          arg0 : array of floats
              Single scattering albedo

          Returns
          -------
          None
          )")

      .def("set_temperature_on_level", &DisortWrapper::SetTemperatureOnLevel,
          R"(
          Set temperature (K) on levels (cell interfaces)

          Parameters
          ----------
          arg0 : array of floats
              Temperature on levels

          Returns
          -------
          None
          )")

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
          }, R"(
          Set layer phase moments

          Parameters
          ----------
          arg0 : array of floats
              Phase moments

          Returns
          -------
          None
          )")

      .def("run", &DisortWrapper::Run, R"(
          Run disort radiative transfer solver

          Parameters
          ----------
          None

          Returns
          -------
          DisortWrapper object
          )")

      //! \todo better api for getting fluxes
      .def("fluxes",
          [](DisortWrapper &disort) {
            py::capsule capsule(&disort.Result()->rad[0].rfldir, [](void *) {});
            py::array_t<double> ndarray({disort.nLayers() + 1, 8}, 
                &disort.Result()->rad[0].rfldir, capsule);
            return ndarray;
          }, R"(
          Get the fluxes from disort

          Parameters
          ----------
          None

          Returns
          -------
          fluxes: numpy.ndarray, shape (nlyr + 1, 8)
              The returned array references the internal memory of the disort object.
              The first dimension is the number of "levels", which is one more than
              the number of layers. The second dimension is the number of flux fields.
              There are 8 flux fields, named as follows:
               - RFLDIR: Direct beam flux
               - FLDN: Diffuse downward flux
               - FLUP: Diffuse upward flux
               - DFDT: Flux divergence, d (net flux) / d (optical depth)
               - UAVG: mean intensity including direct beam
               - UAVGDN: mean diffuse downward intensity
               - UAVGUP: mean diffuse upward intensity
               - UAVGSO: mean direct beam
              All quantities are calculated without delta-M scaling.

          Example
          -------
          >>> from pydisort import disort, RFLDIR, FLDN, FLUP
          >>> ds = pydisort.disort()
          >>> ds.seal()
          >>> flx = ds.run().fluxes()[:, [RFLDIR, FLDN, FLUP]]
          >>> print(disort)
          )")

      .def("intensity",
          [](DisortWrapper &disort) {
            py::capsule capsule(disort.Result()->uu, [](void *) {});
            py::array_t<double> ndarray({disort.nUserAzimuthalAngles(), 
                                         disort.nUserOpticalDepths(),
                                         disort.nUserPolarAngles()}, 
                                         disort.Result()->uu,
                                         capsule);
            return ndarray;
          }, R"(
          Get the intensity from disort

          Parameters
          ----------
          None

          Returns
          -------
          intensity: numpy.ndarray
              Intensity
          )")

      .def("dimensions",
          [](DisortWrapper &disort) {
            std::tuple<int, int, int> shape = {disort.nLayers(), 
                                               disort.nStreams(),
                                               disort.nMoments()};
            return shape;
          }, R"(
          Get the dimension of the disort solver

          Parameters
          ----------
          None

          Returns
          -------
          nlyr : int
              Number of layers
          nstr : int
              Number of streams
          nmom : int
              Number of phase function moments
          )");
}
