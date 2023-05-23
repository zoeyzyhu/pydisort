#include "cppdisort.h"

#include <toml++/toml.h>

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

py::array_t<double> getLegendreCoefficients(int nmom, std::string model,
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
  auto ds = &disort->_ds;
  auto ds_out = &disort->_ds_out;

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

void DisortWrapper::SetHeader(std::string header) {
  snprintf(_ds.header, 1024, "%s", header.c_str());
}

DisortWrapper *DisortWrapper::SetAtmosphereDimension(int nlyr, int nmom,
                                                     int nstr, int nphase) {
  if (_is_finalized) {
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

  _ds.nlyr = nlyr;
  _ds.nmom = nmom;
  _ds.nstr = nstr;
  _ds.nphi = nphase;

  return this;
}

DisortWrapper *DisortWrapper::SetFlags(
    std::map<std::string, bool> const &dict) {
  if (dict.find("ibcnd") != dict.end()) {
    _ds.flag.ibcnd = dict.at("ibcnd");
  }

  if (dict.find("usrtau") != dict.end()) {
    _ds.flag.usrtau = dict.at("usrtau");
  }

  if (dict.find("usrang") != dict.end()) {
    _ds.flag.usrang = dict.at("usrang");
  }

  if (dict.find("lamber") != dict.end()) {
    _ds.flag.lamber = dict.at("lamber");
  }

  if (dict.find("planck") != dict.end()) {
    _ds.flag.planck = dict.at("planck");
  }

  if (dict.find("spher") != dict.end()) {
    _ds.flag.spher = dict.at("spher");
  }

  if (dict.find("onlyfl") != dict.end()) {
    _ds.flag.onlyfl = dict.at("onlyfl");
  }

  if (dict.find("quiet") != dict.end()) {
    _ds.flag.quiet = dict.at("quiet");
  }

  if (dict.find("intensity_correction") != dict.end()) {
    _ds.flag.intensity_correction = dict.at("intensity_correction");
  }

  if (dict.find("old_intensity_correction") != dict.end()) {
    _ds.flag.old_intensity_correction = dict.at("old_intensity_correction");
  }

  if (dict.find("general_source") != dict.end()) {
    _ds.flag.general_source = dict.at("general_source");
  }

  if (dict.find("output_uum") != dict.end()) {
    _ds.flag.output_uum = dict.at("output_uum");
  }

  return this;
}

DisortWrapper *DisortWrapper::SetIntensityDimension(int nuphi, int nutau,
                                                    int numu) {
  if (_is_finalized) {
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

  if (_ds.flag.usrang) {
    _ds.nphi = nuphi;
    _ds.numu = numu;
  }

  if (_ds.flag.usrtau) _ds.ntau = nutau;
  return this;
}

void DisortWrapper::Finalize() {
  if (!_is_finalized) {
    c_disort_state_alloc(&_ds);
    c_disort_out_alloc(&_ds, &_ds_out);
    _is_finalized = true;
  }
}

DisortWrapper::~DisortWrapper() {
  if (_is_finalized) {
    c_disort_state_free(&_ds);
    c_disort_out_free(&_ds, &_ds_out);
    _is_finalized = false;
  }
}

void DisortWrapper::SetOpticalDepth(double *tau, int len) {
  for (int i = 0; i < std::min(_ds.nlyr, len); ++i) {
    _ds.dtauc[i] = tau[i];
  }
}

void DisortWrapper::SetSingleScatteringAlbedo(double *ssa, int len) {
  for (int i = 0; i < std::min(_ds.nlyr, len); ++i) {
    _ds.ssalb[i] = ssa[i];
  }
}

void DisortWrapper::SetLevelTemperature(double *temp, int len) {
  for (int i = 0; i <= std::min(_ds.nlyr, len - 1); ++i) {
    _ds.temper[i] = temp[i];
  }
}

void DisortWrapper::SetUserOpticalDepth(double *usrtau, int len) {
  if (_ds.flag.usrtau) {
    for (int i = 0; i < std::min(_ds.ntau, len); ++i) {
      _ds.utau[i] = usrtau[i];
    }
  }
}

void DisortWrapper::SetUserCosinePolarAngle(double *umu, int len) {
  if (_ds.flag.usrang) {
    for (int i = 0; i < std::min(_ds.numu, len); ++i) {
      _ds.umu[i] = umu[i];
    }
  }
}

void DisortWrapper::SetUserAzimuthalAngle(double *phi, int len) {
  if (_ds.flag.usrang) {
    for (int i = 0; i < std::min(_ds.nphi, len); ++i) {
      _ds.phi[i] = phi[i];
    }
  }
}

void DisortWrapper::SetPhaseMoments(double *pmom, int nlyr, int nmom_p1) {
  std::memcpy(_ds.pmom, pmom, nlyr * nmom_p1 * sizeof(double));
}

py::array_t<double> DisortWrapper::GetFlux() const {
  py::array_t<double> ndarray({_ds.nlyr + 1, 8}, &_ds_out.rad[0].rfldir);
  return ndarray;
}

py::array_t<double> DisortWrapper::GetIntensity() const {
  py::array_t<double> ndarray({_ds.nphi, _ds.ntau, _ds.numu}, _ds_out.uu);
  return ndarray;
}

DisortWrapper *DisortWrapper::Run() {
  if (!_is_finalized) {
    return this;
  }

  _ds.bc.btemp = btemp;
  _ds.bc.ttemp = ttemp;
  _ds.bc.fluor = fluor;
  _ds.bc.albedo = albedo;
  _ds.bc.fisot = fisot;
  _ds.bc.fbeam = fbeam;
  _ds.bc.temis = temis;
  _ds.bc.umu0 = umu0;
  _ds.bc.phi0 = phi0;

  c_disort(&_ds, &_ds_out);

  return this;
}

void DisortWrapper::printDisortState() {
  std::cout << "Leves = " << _ds.nlyr << std::endl;
  std::cout << "Moments = " << _ds.nmom << std::endl;
  std::cout << "Streams = " << _ds.nstr << std::endl;
  std::cout << "Phase functions = " << _ds.nphase << std::endl;
  std::cout << "Accuracy = " << _ds.accur << std::endl;
}
