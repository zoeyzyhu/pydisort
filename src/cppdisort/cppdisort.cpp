// C/C++
#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <cstring>

// pydisort
#include <configure.hpp>

#include "cppdisort.hpp"

std::vector<double> get_phase_function(int nmom, std::string_view model,
                                       double gg) {
  std::vector<double> pmom(1 + nmom);

  if (model == "isotropic") {
    c_getmom(ISOTROPIC, gg, nmom, pmom.data());
  } else if (model == "rayleigh") {
    c_getmom(RAYLEIGH, gg, nmom, pmom.data());
  } else if (model == "henyey_greenstein") {
    c_getmom(HENYEY_GREENSTEIN, gg, nmom, pmom.data());
  } else if (model == "haze_garcia_siewert") {
    c_getmom(HAZE_GARCIA_SIEWERT, gg, nmom, pmom.data());
  } else if (model == "cloud_garcia_siewart") {
    c_getmom(CLOUD_GARCIA_SIEWERT, gg, nmom, pmom.data());
  } else {
    throw std::invalid_argument("invalid scattering model");
  }

  return pmom;
}

void DisortWrapper::SetHeader(std::string const &header) {
  snprintf(ds_.header, sizeof(ds_.header), "%s", header.c_str());
}

DisortWrapper *DisortWrapper::SetAtmosphereDimension(int nlyr, int nstr,
                                                     int nmom, int nphase) {
  if (is_sealed_) {
    throw std::runtime_error(
        "DisortWrapper::SetAtmosphereDimension: "
        "cannot set atmosphere dimension after sealing");
  }

  if (nlyr <= 0) {
    throw std::invalid_argument(
        "DisortWrapper::SetAtmosphereDimension: "
        "number of layers must be positive");
  }

  if (nmom <= 0) {
    throw std::invalid_argument(
        "DisortWrapper::SetAtmosphereDimension: "
        "number of moments must be positive");
  }

  if (nstr <= 0) {
    throw std::invalid_argument(
        "DisortWrapper::SetAtmosphereDimension: "
        "number of streams must be positive");
  }

  if (nphase <= 0) {
    throw std::invalid_argument(
        "DisortWrapper::SetAtmosphereDimension: "
        "number of phase functions must be positive");
  }

  ds_.nlyr = nlyr;
  ds_.nmom = nmom;
  ds_.nstr = nstr;
  ds_.nphase = nphase;

  return this;
}

DisortWrapper *DisortWrapper::SetFlags(
    std::map<std::string, bool> const &dict) {
  if (dict.find("ibcnd") != dict.end()) {
    ds_.flag.ibcnd = dict.at("ibcnd");
  } else {
    ds_.flag.ibcnd = false;
  }

  if (dict.find("usrtau") != dict.end()) {
    ds_.flag.usrtau = dict.at("usrtau");
  } else {
    ds_.flag.usrtau = true;
  }

  if (ds_.flag.usrtau) {
    ds_.ntau = 1;
  }

  if (dict.find("usrang") != dict.end()) {
    ds_.flag.usrang = dict.at("usrang");
  } else {
    ds_.flag.usrang = true;
  }

  if (ds_.flag.usrang) {
    ds_.nphi = 1;
    ds_.numu = 1;
  }

  if (dict.find("lamber") != dict.end()) {
    ds_.flag.lamber = dict.at("lamber");
  } else {
    ds_.flag.lamber = true;
  }

  if (dict.find("planck") != dict.end()) {
    ds_.flag.planck = dict.at("planck");
  } else {
    ds_.flag.planck = true;
  }

  if (dict.find("spher") != dict.end()) {
    ds_.flag.spher = dict.at("spher");
  } else {
    ds_.flag.spher = false;
  }

  if (dict.find("onlyfl") != dict.end()) {
    ds_.flag.onlyfl = dict.at("onlyfl");
  } else {
    ds_.flag.onlyfl = false;
  }

  if (dict.find("quiet") != dict.end()) {
    ds_.flag.quiet = dict.at("quiet");
  } else {
    ds_.flag.quiet = true;
  }

  if (dict.find("intensity_correction") != dict.end()) {
    ds_.flag.intensity_correction = dict.at("intensity_correction");
  } else {
    ds_.flag.intensity_correction = true;
  }

  if (dict.find("old_intensity_correction") != dict.end()) {
    ds_.flag.old_intensity_correction = dict.at("old_intensity_correction");
  } else {
    ds_.flag.old_intensity_correction = true;
  }

  if (dict.find("general_source") != dict.end()) {
    ds_.flag.general_source = dict.at("general_source");
  } else {
    ds_.flag.general_source = false;
  }

  if (dict.find("output_uum") != dict.end()) {
    ds_.flag.output_uum = dict.at("output_uum");
  } else {
    ds_.flag.output_uum = false;
  }

  if (dict.find("print-input") != dict.end()) {
    ds_.flag.prnt[0] = dict.at("print-input");
  } else {
    ds_.flag.prnt[0] = false;
  }

  if (dict.find("print-fluxes") != dict.end()) {
    ds_.flag.prnt[1] = dict.at("print-fluxes");
  } else {
    ds_.flag.prnt[1] = false;
  }

  if (dict.find("print-intensity") != dict.end()) {
    ds_.flag.prnt[2] = dict.at("print-intensity");
  } else {
    ds_.flag.prnt[2] = false;
  }

  if (dict.find("print-transmissivity") != dict.end()) {
    ds_.flag.prnt[3] = dict.at("print-transmissivity");
  } else {
    ds_.flag.prnt[3] = false;
  }

  if (dict.find("print-phase-function") != dict.end()) {
    ds_.flag.prnt[4] = dict.at("print-phase-function");
  } else {
    ds_.flag.prnt[4] = false;
  }

  return this;
}

DisortWrapper *DisortWrapper::SetIntensityDimension(int nuphi, int nutau,
                                                    int numu) {
  if (is_sealed_) {
    throw std::runtime_error(
        "DisortWrapper::SetIntensityDimension: "
        "cannot set intensity dimension after sealing");
  }

  if (nuphi <= 0) {
    throw std::invalid_argument(
        "DisortWrapper::SetIntensityDimension: "
        "nuphi must be positive");
  }

  if (numu <= 0) {
    throw std::invalid_argument(
        "DisortWrapper::SetIntensityDimension: "
        "numu must be positive");
  }

  if (nutau <= 0) {
    throw std::invalid_argument(
        "DisortWrapper::SetIntensityDimension: "
        "nutau must be positive");
  }

  if (ds_.flag.usrang) {
    ds_.nphi = nuphi;
    ds_.numu = numu;
  }

  if (ds_.flag.usrtau) ds_.ntau = nutau;
  return this;
}

void DisortWrapper::Seal() {
  if (!is_sealed_) {
    c_disort_state_alloc(&ds_);
    c_disort_out_alloc(&ds_, &ds_out_);

    if (ds_.flag.usrtau) {
      ds_.utau[0] = 0.0;
    }

    if (ds_.flag.usrang) {
      ds_.umu[0] = 1.0;
      ds_.phi[0] = 0.0;
    }

    is_sealed_ = true;
  }
}

void DisortWrapper::Unseal() {
  if (is_sealed_) {
    c_disort_state_free(&ds_);
    c_disort_out_free(&ds_, &ds_out_);
    is_sealed_ = false;
  }
}

DisortWrapper::~DisortWrapper() {
  if (is_sealed_) {
    c_disort_state_free(&ds_);
    c_disort_out_free(&ds_, &ds_out_);
    is_sealed_ = false;
  }
}

void DisortWrapper::SetOpticalThickness(std::vector<double> const &tau) {
  for (int i = 0; i < std::min((size_t)ds_.nlyr, tau.size()); ++i) {
    if (tau[i] < 0) {
      throw std::runtime_error("DisortWrapper::SetOpticalThickness: "
                               "Optical thickness must be non-negative.");
    }
    ds_.dtauc[i] = tau[i];
  }
}

void DisortWrapper::SetSingleScatteringAlbedo(std::vector<double> const &ssa) {
  for (int i = 0; i < std::min((size_t)ds_.nlyr, ssa.size()); ++i) {
    if (ssa[i] < 0 || ssa[i] > 1) {
      throw std::runtime_error("DisortWrapper::SetSingleScatteringAlbedo: "
                               "Single scattering albedo must be in [0, 1].");
    }
    ds_.ssalb[i] = ssa[i];
  }
}

void DisortWrapper::SetTemperatureOnLevel(std::vector<double> const &temp) {
  for (int i = 0; i <= std::min((size_t)ds_.nlyr, temp.size() - 1); ++i) {
    if (temp[i] < 0) {
      throw std::runtime_error("DisortWrapper::SetTemperatureOnLevel: "
                               "Temperature must be positive.");
    }
    ds_.temper[i] = temp[i];
  }
}

void DisortWrapper::SetUserOpticalDepth(std::vector<double> const &utau) {
  if (!is_sealed_) {
    throw std::runtime_error("DisortWrapper::SetUserOpticalDepth: "
                             "DisortWrapper is not sealed. Call seal() first.");
  }

  if (ds_.flag.usrtau) {
    for (int i = 0; i < std::min((size_t)ds_.ntau, utau.size()); ++i) {
      if (utau[i] < 0) {
        throw std::runtime_error("DisortWrapper::SetUserOpticalDepth: "
                                 "Optical depth must be non-negative.");
      }
      ds_.utau[i] = utau[i];
    }
  }
}

void DisortWrapper::SetUserCosinePolarAngle(std::vector<double> const &umu) {
  if (ds_.flag.usrang) {
    for (int i = 0; i < std::min((size_t)ds_.numu, umu.size()); ++i) {
      if (umu[i] < -1 || umu[i] > 1) {
        throw std::runtime_error("DisortWrapper::SetUserCosinePolarAngle: "
                                 "Cosine of polar angle must be in [-1, 1].");
      }
      ds_.umu[i] = umu[i];
    }
  }
}

void DisortWrapper::SetUserAzimuthalAngle(std::vector<double> const &uphi) {
  if (ds_.flag.usrang) {
    for (int i = 0; i < std::min((size_t)ds_.nphi, uphi.size()); ++i) {
      ds_.phi[i] = uphi[i];
    }
  }
}

void DisortWrapper::SetPhaseMoments(double *pmom, int nlyr, int nmom_p1) {
  std::memcpy(ds_.pmom, pmom, nlyr * nmom_p1 * sizeof(double));
}

DisortWrapper *DisortWrapper::Run() {
  if (!is_sealed_) {
    throw std::runtime_error("DisortWrapper::Run: "
                             "DisortWrapper is not sealed. Call seal() first.");
  }

  ds_.bc.btemp = btemp;
  ds_.bc.ttemp = ttemp;
  ds_.bc.fluor = fluor;
  ds_.bc.albedo = albedo;
  ds_.bc.fisot = fisot;
  ds_.bc.fbeam = fbeam;
  ds_.bc.temis = temis;
  ds_.bc.umu0 = umu0;
  ds_.bc.phi0 = phi0;

  c_disort(&ds_, &ds_out_);

  return this;
}

void DisortWrapper::printDisortAtmosphere(std::ostream &os) const {
  os << "- Levels = " << ds_.nlyr << std::endl;
  os << "- Moments = " << ds_.nmom << std::endl;
  os << "- Streams = " << ds_.nstr << std::endl;
  os << "- Phase functions = " << ds_.nphase << std::endl;
}

void DisortWrapper::printDisortOutput(std::ostream &os) const {
  os << "- User azimuthal angles = " << ds_.nphi << std::endl;
  for (int i = 0; i < ds_.nphi; ++i) {
    os << "  : " << ds_.phi[i] / M_PI * 180. << ", ";
  }
  os << std::endl;
  os << "- User polar angles = " << ds_.numu << std::endl;
  for (int i = 0; i < ds_.numu; ++i) {
    os << "  : " << acos(ds_.umu[i]) / M_PI * 180. << ", ";
  }
  os << std::endl;
  os << "- User optical depths = " << ds_.ntau << std::endl;
  for (int i = 0; i < ds_.ntau; ++i) {
    os << "  : " << ds_.utau[i] << ", ";
  }
  os << std::endl;
}

void DisortWrapper::printDisortFlags(std::ostream &os) const {
  if (ds_.flag.ibcnd) {
    os << "- Spectral boundary condition (ibcnd) = True" << std::endl;
  } else {
    os << "- Spectral boundary condition (ibcnd) = False" << std::endl;
  }

  if (ds_.flag.usrtau) {
    os << "- User optical depth (usrtau) = True" << std::endl;
  } else {
    os << "- User optical depth (usrtau) = False" << std::endl;
  }

  if (ds_.flag.usrang) {
    os << "- User angles (usrang) = True" << std::endl;
  } else {
    os << "- User angles (usrang) = False" << std::endl;
  }

  if (ds_.flag.lamber) {
    os << "- Lambertian surface (lamber) = True" << std::endl;
  } else {
    os << "- Lambertian surface (lamber) = False" << std::endl;
  }

  if (ds_.flag.planck) {
    os << "- Planck function (planck) = True" << std::endl;
  } else {
    os << "- Planck function (planck) = False" << std::endl;
  }

  if (ds_.flag.spher) {
    os << "- Spherical correction (spher) = True" << std::endl;
  } else {
    os << "- Spherical correction (spher) = False" << std::endl;
  }

  if (ds_.flag.onlyfl) {
    os << "- Only calculate fluxes (onlyfl) = True" << std::endl;
  } else {
    os << "- Only calculate fluxes (onlyfl) = False" << std::endl;
  }

  if (ds_.flag.intensity_correction) {
    os << "- Intensity correction (intensity_correction) = True" << std::endl;
  } else {
    os << "- Intensity correction (intensity_correction) = False" << std::endl;
  }

  if (ds_.flag.old_intensity_correction) {
    os << "- Old intensity correction (old_intensity_correction) = True"
       << std::endl;
  } else {
    os << "- Old intensity correction (old_intensity_correction) = False"
       << std::endl;
  }

  if (ds_.flag.general_source) {
    os << "- General source function (general_source) = True" << std::endl;
  } else {
    os << "- General source function (general_source) = False" << std::endl;
  }

  if (ds_.flag.output_uum) {
    os << "- Output uum (output_uum) = True" << std::endl;
  } else {
    os << "- Output uum (output_uum) = False" << std::endl;
  }
}

std::string DisortWrapper::ToString() const {
  std::stringstream ss;

  ss << "Disort is configured with:" << std::endl;
  printDisortFlags(ss);
  ss << "- Accuracy = " << ds_.accur << std::endl;

  if (is_sealed_) {
    printDisortAtmosphere(ss);
    printDisortOutput(ss);
    ss << "Disort is finalized.";
  } else {
    ss << "Disort is not yet finalized.";
  }

  return ss.str();
}
