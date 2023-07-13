#include "cppdisort.hpp"

#include <toml++/toml.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>

const int Radiant::RFLDIR;
const int Radiant::FLDN;
const int Radiant::FLUP;
const int Radiant::DFDT;
const int Radiant::UAVG;
const int Radiant::UAVGDN;
const int Radiant::UAVGUP;
const int Radiant::UAVGSO;

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

  ds->flag.usrtau = table["flag"]["usrtau"].value<bool>().value_or(false);

  ds->flag.usrang = table["flag"]["usrang"].value<bool>().value_or(false);

  return disort;
}

void DisortWrapper::SetHeader(std::string const &header) {
  snprintf(ds.header_, sizeof(ds.header_), "%s", header.c_str());
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

  ds.nlyr_ = nlyr;
  ds.nmom_ = nmom;
  ds.nstr_ = nstr;
  ds.nphi_ = nphase;

  return this;
}

DisortWrapper *DisortWrapper::SetFlags(
    std::map<std::string, bool> const &dict) {
  if (dict.find("ibcnd") != dict.end()) {
    ds.flag.ibcnd_ = dict.at("ibcnd");
  }

  if (dict.find("usrtau") != dict.end()) {
    ds.flag.usrtau_ = dict.at("usrtau");
  }

  if (dict.find("usrang") != dict.end()) {
    ds.flag.usrang_ = dict.at("usrang");
  }

  if (dict.find("lamber") != dict.end()) {
    ds.flag.lamber_ = dict.at("lamber");
  }

  if (dict.find("planck") != dict.end()) {
    ds.flag.planck_ = dict.at("planck");
  }

  if (dict.find("spher") != dict.end()) {
    ds.flag.spher_ = dict.at("spher");
  }

  if (dict.find("onlyfl") != dict.end()) {
    ds.flag.onlyfl_ = dict.at("onlyfl");
  }

  if (dict.find("quiet") != dict.end()) {
    ds.flag.quiet_ = dict.at("quiet");
  }

  if (dict.find("intensity_correction") != dict.end()) {
    ds.flag.intensity_correction_ = dict.at("intensity_correction");
  }

  if (dict.find("old_intensity_correction") != dict.end()) {
    ds.flag.old_intensity_correction_ = dict.at("old_intensity_correction");
  }

  if (dict.find("general_source") != dict.end()) {
    ds.flag.general_source_ = dict.at("general_source");
  }

  if (dict.find("output_uum") != dict.end()) {
    ds.flag.output_uum_ = dict.at("output_uum");
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

  if (ds.flag.usrang_) {
    ds.nphi_ = nuphi;
    ds.numu_ = numu;
  }

  if (ds.flag.usrtau_) ds.ntau_ = nutau;
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
  for (int i = 0; i < std::min(ds.nlyr_, len); ++i) {
    ds.dtauc_[i] = tau[i];
  }
}

void DisortWrapper::SetSingleScatteringAlbedo(double const *ssa, int len) {
  for (int i = 0; i < std::min(ds.nlyr_, len); ++i) {
    ds.ssalb_[i] = ssa[i];
  }
}

void DisortWrapper::SetLevelTemperature(double const *temp, int len) {
  for (int i = 0; i <= std::min(ds.nlyr_, len - 1); ++i) {
    ds.temper_[i] = temp[i];
  }
}

void DisortWrapper::SetUserOpticalDepth(double const *usrtau, int len) {
  if (ds.flag.usrtau_) {
    for (int i = 0; i < std::min(ds.ntau_, len); ++i) {
      ds.utau_[i] = usrtau[i];
    }
  }
}

void DisortWrapper::SetUserCosinePolarAngle(double const *umu, int len) {
  if (ds.flag.usrang_) {
    for (int i = 0; i < std::min(ds.numu_, len); ++i) {
      ds.umu_[i] = umu[i];
    }
  }
}

void DisortWrapper::SetUserAzimuthalAngle(double const *phi, int len) {
  if (ds.flag.usrang_) {
    for (int i = 0; i < std::min(ds.nphi_, len); ++i) {
      ds.phi_[i] = phi[i];
    }
  }
}

void DisortWrapper::SetPhaseMoments(double *pmom, int nlyr, int nmom_p1) {
  std::memcpy(ds.pmom_, pmom, nlyr * nmom_p1 * sizeof(double));
}

py::array_t<double> DisortWrapper::GetFlux() const {
  py::array_t<double> ndarray({ds.nlyr_ + 1, 8}, &ds_out_.rad[0].rfldir);
  return ndarray;
}

py::array_t<double> DisortWrapper::GetIntensity() const {
  py::array_t<double> ndarray({ds.nphi_, ds.ntau_, ds.numu_}, ds_out_.uu);
  return ndarray;
}

DisortWrapper *DisortWrapper::Run() {
  if (!is_finalized_) {
    return this;
  }

  ds.bc.btemp_ = btemp;
  ds.bc.ttemp_ = ttemp;
  ds.bc.fluor_ = fluor;
  ds.bc.albedo_ = albedo;
  ds.bc.fisot_ = fisot;
  ds.bc.fbeam_ = fbeam;
  ds.bc.temis_ = temis;
  ds.bc.umu0_ = umu0;
  ds.bc.phi0_ = phi0;

  c_disort(&ds_, &ds_out_);

  return this;
}

void DisortWrapper::printDisortAtmosphere(std::ostream &os) const {
  os << "- Levels = " << ds.nlyr_ << std::endl;
  os << "- Moments = " << ds.nmom_ << std::endl;
  os << "- Streams = " << ds.nstr_ << std::endl;
  os << "- Phase functions = " << ds.nphase_ << std::endl;
}

void DisortWrapper::printDisortOutput(std::ostream &os) const {
  os << "- User azimuthal angles = " << ds.nphi_ << std::endl;
  os << "- User polar angles = " << ds.numu_ << std::endl;
  os << "- User optical depths = " << ds.ntau_ << std::endl;
}

void DisortWrapper::printDisortFlags(std::ostream &os) const {
  if (ds.flag.ibcnd_) {
    os << "- Spectral boundary condition (ibcnd) = True" << std::endl;
  } else {
    os << "- Spectral boundary condition (ibcnd) = False" << std::endl;
  }

  if (ds.flag.usrtau_) {
    os << "- User optical depth (usrtau) = True" << std::endl;
  } else {
    os << "- User optical depth (usrtau) = False" << std::endl;
  }

  if (ds.flag.usrang__) {
    os << "- User angles (usrang) = True" << std::endl;
  } else {
    os << "- User angles (usrang) = False" << std::endl;
  }

  if (ds.flag.lamber_) {
    os << "- Lambertian surface (lamber) = True" << std::endl;
  } else {
    os << "- Lambertian surface (lamber) = False" << std::endl;
  }

  if (ds.flag.planck_) {
    os << "- Planck function (planck) = True" << std::endl;
  } else {
    os << "- Planck function (planck) = False" << std::endl;
  }

  if (ds.flag.spher_) {
    os << "- Spherical correction (spher) = True" << std::endl;
  } else {
    os << "- Spherical correction (spher) = False" << std::endl;
  }

  if (ds.flag.onlyfl_) {
    os << "- Only calculate fluxes (onlyfl) = True" << std::endl;
  } else {
    os << "- Only calculate fluxes (onlyfl) = False" << std::endl;
  }

  if (ds.flag.intensity_correction_) {
    os << "- Intensity correction (intensity_correction) = True" << std::endl;
  } else {
    os << "- Intensity correction (intensity_correction) = False" << std::endl;
  }

  if (ds.flag.old_intensity_correction_) {
    os << "- Old intensity correction (old_intensity_correction) = True"
       << std::endl;
  } else {
    os << "- Old intensity correction (old_intensity_correction) = False"
       << std::endl;
  }

  if (ds.flag.general_source_) {
    os << "- General source function (general_source) = True" << std::endl;
  } else {
    os << "- General source function (general_source) = False" << std::endl;
  }

  if (ds.flag.output_uum_) {
    os << "- Output uum (output_uum) = True" << std::endl;
  } else {
    os << "- Output uum (output_uum) = False" << std::endl;
  }
}

std::string DisortWrapper::ToString() const {
  std::stringstream ss;

  ss << "Disort is configured with:" << std::endl;
  printDisortFlags(ss);
  ss << "- Accuracy = " << ds.accur_ << std::endl;

  if (is_finalized_) {
    printDisortAtmosphere(ss);
    printDisortOutput(ss);
    ss << "Disort is finalized." << std::endl;
  } else {
    ss << "Disort is not yet finalized." << std::endl;
  }

  return ss.str();
}
