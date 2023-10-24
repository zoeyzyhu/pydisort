// toml
#include <toml++/toml.h>

// C/C++
#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>

// pydisort
#include <configure.hpp>

#include "cppdisort.hpp"

const int Radiant::RFLDIR;
const int Radiant::FLDN;
const int Radiant::FLUP;
const int Radiant::DFDT;
const int Radiant::UAVG;
const int Radiant::UAVGDN;
const int Radiant::UAVGUP;
const int Radiant::UAVGSO;

#ifdef PYTHON_BINDINGS
py::array_t<double> getLegendreCoefficients(int nmom, std::string const &model,
                                            double gg) {
  py::array_t<double> py_pmom(1 + nmom);
  double *ptr = static_cast<double *>(py_pmom.request().ptr);
  std::memset(ptr, 0, sizeof(double) * (1 + nmom));

  if (model == "isotropic") {
    c_getmom(ISOTROPIC, gg, nmom, ptr);
  } else if (model == "rayleigh") {
    c_getmom(RAYLEIGH, gg, nmom, ptr);
  } else if (model == "henyey_greenstein") {
    c_getmom(HENYEY_GREENSTEIN, gg, nmom, ptr);
  } else if (model == "haze_garcia_siewert") {
    c_getmom(HAZE_GARCIA_SIEWERT, gg, nmom, ptr);
  } else if (model == "cloud_garcia_siewart") {
    c_getmom(CLOUD_GARCIA_SIEWERT, gg, nmom, ptr);
  } else {
    throw std::invalid_argument("invalid scattering model");
  }

  return py_pmom;
}
#endif  // PYTHON_BINDINGS

DisortWrapper *DisortWrapper::fromTomlTable(const toml::table &table) {
  auto disort = new DisortWrapper();
  auto ds = &disort->ds_;
  auto ds_out = &disort->ds_out_;

  ds->flag.ibcnd = table["flag"]["ibcnd"].value<bool>().value_or(false);
  ds->flag.usrtau = table["flag"]["usrtau"].value<bool>().value_or(false);
  ds->flag.usrang = table["flag"]["usrang"].value<bool>().value_or(false);
  ds->flag.lamber = table["flag"]["lamber"].value<bool>().value_or(false);
  ds->flag.planck = table["flag"]["planck"].value<bool>().value_or(false);
  ds->flag.spher = table["flag"]["spher"].value<bool>().value_or(false);
  ds->flag.onlyfl = table["flag"]["onlyfl"].value<bool>().value_or(false);
  ds->flag.quiet = table["flag"]["quiet"].value<bool>().value_or(false);
  ds->flag.brdf_type = table["flag"]["brdf_type"].value<int>().value_or(0);
  ds->flag.intensity_correction =
      table["flag"]["intensity_correction"].value<bool>().value_or(false);
  ds->flag.old_intensity_correction =
      table["flag"]["old_intensity_correction"].value<bool>().value_or(false);
  ds->flag.general_source =
      table["flag"]["general_source"].value<bool>().value_or(false);
  ds->flag.output_uum =
      table["flag"]["output_uum"].value<bool>().value_or(false);

  for (int i = 0; i < 5; ++i) {
    ds->flag.prnt[i] = table["flag"]["prnt"][i].value<bool>().value_or(false);
  }

  return disort;
}

void DisortWrapper::SetHeader(std::string const &header) {
  snprintf(ds_.header, sizeof(ds_.header), "%s", header.c_str());
}

DisortWrapper *DisortWrapper::SetAtmosphereDimension(int nlyr, int nstr,
                                                     int nmom, int nphase) {
  if (is_finalized_) {
    return this;
  }

  if (nlyr <= 0) {
    return this;
  }

  if (nmom <= 0) {
    return this;
  }

  if (nstr <= 0) {
    return this;
  }

  if (nphase <= 0) {
    return this;
  }

  ds_.nlyr = nlyr;
  ds_.nmom = nmom;
  ds_.nstr = nstr;
  ds_.nphi = nphase;

  return this;
}

DisortWrapper *DisortWrapper::SetFlags(
    std::map<std::string, bool> const &dict) {
  if (dict.find("ibcnd") != dict.end()) {
    ds_.flag.ibcnd = dict.at("ibcnd");
  }

  if (dict.find("usrtau") != dict.end()) {
    ds_.flag.usrtau = dict.at("usrtau");
  }

  if (dict.find("usrang") != dict.end()) {
    ds_.flag.usrang = dict.at("usrang");
  }

  if (dict.find("lamber") != dict.end()) {
    ds_.flag.lamber = dict.at("lamber");
  }

  if (dict.find("planck") != dict.end()) {
    ds_.flag.planck = dict.at("planck");
  }

  if (dict.find("spher") != dict.end()) {
    ds_.flag.spher = dict.at("spher");
  }

  if (dict.find("onlyfl") != dict.end()) {
    ds_.flag.onlyfl = dict.at("onlyfl");
  }

  if (dict.find("quiet") != dict.end()) {
    ds_.flag.quiet = dict.at("quiet");
  }

  if (dict.find("intensity_correction") != dict.end()) {
    ds_.flag.intensity_correction = dict.at("intensity_correction");
  }

  if (dict.find("old_intensity_correction") != dict.end()) {
    ds_.flag.old_intensity_correction = dict.at("old_intensity_correction");
  }

  if (dict.find("general_source") != dict.end()) {
    ds_.flag.general_source = dict.at("general_source");
  }

  if (dict.find("output_uum") != dict.end()) {
    ds_.flag.output_uum = dict.at("output_uum");
  }

  return this;
}

DisortWrapper *DisortWrapper::SetIntensityDimension(int nuphi, int nutau,
                                                    int numu) {
  if (is_finalized_) {
    return this;
  }

  if (nuphi <= 0) {
    return this;
  }

  if (numu <= 0) {
    return this;
  }

  if (nutau <= 0) {
    return this;
  }

  if (ds_.flag.usrang) {
    ds_.nphi = nuphi;
    ds_.numu = numu;
  }

  if (ds_.flag.usrtau) ds_.ntau = nutau;
  return this;
}

void DisortWrapper::Finalize() {
  if (!is_finalized_) {
    c_disort_state_alloc(&ds_);
    c_disort_out_alloc(&ds_, &ds_out_);
    is_finalized_ = true;
  }
}

DisortWrapper::~DisortWrapper() {
  if (is_finalized_) {
    c_disort_state_free(&ds_);
    c_disort_out_free(&ds_, &ds_out_);
    is_finalized_ = false;
  }
}

void DisortWrapper::SetOpticalDepth(double const *tau, int len) {
  for (int i = 0; i < std::min(ds_.nlyr, len); ++i) {
    ds_.dtauc[i] = tau[i];
  }
}

void DisortWrapper::SetSingleScatteringAlbedo(double const *ssa, int len) {
  for (int i = 0; i < std::min(ds_.nlyr, len); ++i) {
    ds_.ssalb[i] = ssa[i];
  }
}

void DisortWrapper::SetLevelTemperature(double const *temp, int len) {
  for (int i = 0; i <= std::min(ds_.nlyr, len - 1); ++i) {
    ds_.temper[i] = temp[i];
  }
}

void DisortWrapper::SetUserOpticalDepth(double const *usrtau, int len) {
  if (ds_.flag.usrtau) {
    for (int i = 0; i < std::min(ds_.ntau, len); ++i) {
      ds_.utau[i] = usrtau[i];
    }
  }
}

void DisortWrapper::SetUserCosinePolarAngle(double const *umu, int len) {
  if (ds_.flag.usrang) {
    for (int i = 0; i < std::min(ds_.numu, len); ++i) {
      ds_.umu[i] = umu[i];
    }
  }
}

void DisortWrapper::SetUserAzimuthalAngle(double const *phi, int len) {
  if (ds_.flag.usrang) {
    for (int i = 0; i < std::min(ds_.nphi, len); ++i) {
      ds_.phi[i] = phi[i];
    }
  }
}

void DisortWrapper::SetPhaseMoments(double *pmom, int nlyr, int nmom_p1) {
  std::memcpy(ds_.pmom, pmom, nlyr * nmom_p1 * sizeof(double));
}

#ifdef PYTHON_BINDINGS
py::array_t<double> DisortWrapper::GetFlux() const {
  py::array_t<double> ndarray({ds_.nlyr + 1, 8}, &ds_out_.rad[0].rfldir);
  return ndarray;
}

py::array_t<double> DisortWrapper::GetIntensity() const {
  py::array_t<double> ndarray({ds_.nphi, ds_.ntau, ds_.numu}, ds_out_.uu);
  return ndarray;
}
#endif  // PYTHON_BINDINGS

DisortWrapper *DisortWrapper::Run() {
  if (!is_finalized_) {
    return this;
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
  os << "- User polar angles = " << ds_.numu << std::endl;
  os << "- User optical depths = " << ds_.ntau << std::endl;
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

  if (is_finalized_) {
    printDisortAtmosphere(ss);
    printDisortOutput(ss);
    ss << "Disort is finalized." << std::endl;
  } else {
    ss << "Disort is not yet finalized." << std::endl;
  }

  return ss.str();
}
