#ifndef INTERFACE_CPPDISORT_HPP_
#define INTERFACE_CPPDISORT_HPP_

// C/C++
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct disort_state;
struct disort_output;

// wraps c_getmom
std::vector<double> get_phase_function(int nmom, std::string model,
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
  DisortWrapper();
  virtual ~DisortWrapper();

 public:  // string method (used in python wrapper)
  std::string ToString() const;

 public:  // setters and getters
  void SetHeader(std::string const &header);
  void SetFlags(std::map<std::string, bool> const &flags);
  void SetAccuracy(double accur);

  void SetIntensityDimension(int nuphi, int nutau, int numu);
  void SetAtmosphereDimension(int nlyr, int nstr, int nmom);

  void Seal();
  void Unseal();
  bool IsSealed() const { return is_sealed_; }
  bool IsFluxOnly() const;

  int nLayers() const;
  int nMoments() const;
  int nStreams() const;
  int nUserOpticalDepths() const;
  int nUserPolarAngles() const;
  int nUserAzimuthalAngles() const;

  //! \brief Set the wavenumber range
  void SetWavenumberRange_invcm(double wmin, double wmax);

  //! \brief Set the wavenumber
  void SetWavenumber_invcm(double wave);

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
  void Run();

  //! \brief Get disort run result
  disort_output const *Result() const;

 protected:
  disort_state *ds_;
  disort_output *ds_out_;

  bool is_sealed_ = false;

  void printDisortAtmosphere(std::ostream &os) const;
  void printDisortOutput(std::ostream &os) const;
  void printDisortFlags(std::ostream &os) const;
  void printBoundaryConditions(std::ostream &os) const;
};

#endif  // SRC_CPPDISORT_CPPDISORT_HPP_
