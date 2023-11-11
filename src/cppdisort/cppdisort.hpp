#ifndef SRC_CPPDISORT_CPPDISORT_HPP_
#define SRC_CPPDISORT_CPPDISORT_HPP_

// C/C++
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

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

// wraps c_getmom
std::vector<double> get_phase_function(int nmom, std::string_view model,
                                       double gg = 0.);

// wraps disort_state and disort_output
class DisortWrapper {
 public: // accessible boundary conditions
  double btemp;
  double ttemp;
  double fluor;
  double albedo;
  double fisot;
  double fbeam;
  double temis;
  double umu0;
  double phi0;

public: // constructor and destructor
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
    SetAccuracy(1.E-6);
  }
  virtual ~DisortWrapper();

  //! \todo will be removed
  static DisortWrapper *FromFile(std::string_view filename) {
    return fromTomlTable(toml::parse_file(filename));
  }

 public:  // string method (used in python wrapper)
  std::string ToString() const;

 public:  // setters and getters
  void SetHeader(std::string const &header);
  DisortWrapper *SetFlags(std::map<std::string, bool> const &flags);
  void SetAccuracy(double accur) { ds_.accur = accur; }

  DisortWrapper *SetIntensityDimension(int nuphi, int nutau, int numu);
  DisortWrapper *SetAtmosphereDimension(int nlyr, int nstr, int nmom,
                                        int nphase);

  void Seal();
  void Unseal();
  bool IsSealed() const { return is_sealed_; }

  int nLayers() const { return ds_.nlyr; }
  int nMoments() const { return ds_.nmom; }
  int nStreams() const { return ds_.nstr; }
  int nUserOpticalDepths() const { return ds_.ntau; }
  int nUserPolarAngles() const { return ds_.numu; }
  int nUserAzimuthalAngles() const { return ds_.nphi; }

  //! \brief Set the wavenumber range
  void SetWavenumberRange_invcm(double wmin, double wmax) {
    ds_.wvnmlo = wmin;
    ds_.wvnmhi = wmax;
  }

  //! \brief Set the wavenumber
  void SetWavenumber_invcm(double wave) {
    ds_.wvnmlo = wave;
    ds_.wvnmhi = wave;
  }

  //! \brief Set the optical thickness defined on layers
  void SetOpticalThickness(std::vector<double> const &tau);

  //! \brief Set the single scattering albedo defined on layers
  void SetSingleScatteringAlbedo(std::vector<double> const &ssa);

  //! \brief Set temperatures defined on levels
  void SetTemperatureOnLevel(std::vector<double> const &temp);

  //! \brief Set the optical depth of the output radiance 
  void SetUserOpticalDepth(std::vector<double> const &utau);

  //! \brief Set the cosine polar angle of the output radiance
  void SetUserCosinePolarAngle(std::vector<double> const &umu);

  //! \brief Set the azimuthal angle of the output radiance
  void SetUserAzimuthalAngle(std::vector<double> const &uphi);

  void SetPlanckSource(double *planck);

  //! \brief Set the phase function moments
  //!
  //! pmom is a 1D array of length nlyr * (nmom + 1)
  //! with nlyr being the number of layers and nmom the number of scattering
  //! moments
  void SetPhaseMoments(double *pmom, int nlyr, int nmom_p1);

  //! \brief Run disort
  DisortWrapper *Run();

  //! \brief Get disort run result
  disort_output const *Result() const { return &ds_out_; }

 protected:
  disort_state ds_;
  disort_output ds_out_;

  bool is_sealed_ = false;
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
