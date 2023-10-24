#ifndef SRC_CPPDISORT_CPPDISORT_HPP_
#define SRC_CPPDISORT_CPPDISORT_HPP_

// C/C++
#include <iostream>
#include <map>
#include <string>
#include <tuple>

// toml
#include <toml++/toml.h>

// cdisort
#include <cdisort/cdisort.h>
#undef DEG
#undef SQR
#undef MIN
#undef MAX
#undef LIMIT_RANGE
#undef IMIN
#undef IMAX
#undef F77_SIGN

// pydisort
#include <configure.hpp>

#ifdef PYTHON_BINDINGS
// pybind11
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

// wraps c_getmom
py::array_t<double> getLegendreCoefficients(int nmom, std::string const &model,
                                            double gg = 0.);
#endif  // PYTHON_BINDINGS

// flux index constants
struct Radiant {
  // Direct-beam flux (w/o delta-M scaling)
  static const int RFLDIR = 0;

  // Diffuse down-flux (tot.-direct-beam; w/o delta-M scaling)
  static const int FLDN = 1;

  // Diffuse up-flux
  static const int FLUP = 2;

  // Flux divergence, d(net flux)/d(optical depth)
  static const int DFDT = 3;

  // Mean intensity, incl. direct beam (not corr. for delta-M scaling)
  static const int UAVG = 4;

  // Mean diffuse downward intensity, not incl. direct beam
  // (not corr. for delta-M scaling)
  static const int UAVGDN = 5;

  // Mean diffuse downward intensity, not incl. direct beam
  // (not corr. for delta-M scaling)
  static const int UAVGUP = 6;

  // Mean diffuse direct solar, that is the direct beam
  // (not corr. for delta-M scaling)
  static const int UAVGSO = 7;
};

// wraps disort_state and disort_output
class DisortWrapper {
 public:
  // accessible boundary conditions
  double btemp;
  double ttemp;
  double fluor;
  double albedo;
  double fisot;
  double fbeam;
  double temis;
  double umu0;
  double phi0;

  DisortWrapper()
      : btemp(0.0),
        ttemp(0.0),
        fluor(0.0),
        albedo(0.0),
        fisot(0.0),
        fbeam(0.0),
        temis(0.0),
        umu0(1.0),
        phi0(0.0) {
    ds_.accur = 1.E-6;
  }

  static DisortWrapper *FromFile(std::string_view filename) {
    return fromTomlTable(toml::parse_file(filename));
  }

  std::string ToString() const;

  virtual ~DisortWrapper();

  void SetHeader(std::string const &header);

  DisortWrapper *SetAtmosphereDimension(int nlyr, int nstr, int nmom,
                                        int nphase);

  DisortWrapper *SetFlags(std::map<std::string, bool> const &flags);

  DisortWrapper *SetIntensityDimension(int nuphi, int nutau, int numu);

  void Finalize();

  bool IsFinalized() const { return is_finalized_; }

  int nLayers() const { return ds_.nlyr; }

  int nMoments() const { return ds_.nmom; }

  int nStreams() const { return ds_.nstr; }

  void SetAccuracy(double accur) { ds_.accur = accur; }

  void SetWavenumberRange_invcm(double wmin, double wmax) {
    ds_.wvnmlo = wmin;
    ds_.wvnmhi = wmax;
  }

  void SetWavenumber_invcm(double wave) {
    ds_.wvnmlo = wave;
    ds_.wvnmhi = wave;
  }

  void SetOpticalDepth(double const *tau, int len);

  void SetSingleScatteringAlbedo(double const *ssa, int len);

  // temperature array is defined on levels
  void SetLevelTemperature(double const *temp, int len);

  void SetUserOpticalDepth(double const *usrtau, int len);

  void SetUserCosinePolarAngle(double const *umu, int len);

  void SetUserAzimuthalAngle(double const *phi, int len);

  void SetPlanckSource(double *planck);

  // pmom is a 1D array of length nlyr * (nmom + 1)
  // with nlyr being the number of layers and nmom the number of scattering
  // moments
  void SetPhaseMoments(double *pmom, int nlyr, int nmom_p1);

  DisortWrapper *Run();

#ifdef PYTHON_BINDINGS
  // \todo how to make them actually constant ?
  py::array_t<double> GetFlux() const;
  py::array_t<double> GetIntensity() const;
#endif  // PYTHON_BINDINGS

 protected:
  disort_state ds_;
  disort_output ds_out_;

  bool is_finalized_ = false;
  static DisortWrapper *fromTomlTable(const toml::table &table);

  void printDisortAtmosphere(std::ostream &os) const;
  void printDisortOutput(std::ostream &os) const;
  void printDisortFlags(std::ostream &os) const;
};

// exposing private members for testing
class DisortWrapperTestOnly : public DisortWrapper {
 public:
  static DisortWrapper *FromString(std::string_view content) {
    return fromTomlTable(toml::parse(content));
  }

  disort_state *GetDisortState() { return &ds_; }
  disort_output *GetDisortOutput() { return &ds_out_; }
};

#endif  // SRC_CPPDISORT_CPPDISORT_HPP_
