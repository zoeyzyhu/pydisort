//#include <glog/logging.h>

#include <memory>
#include <iostream>
#include <sstream>
#include <toml++/toml.h>
#include "cppdisort.h"

py::array_t<double> getLegendreCoefficients(double gg, int nmom, std::string model)
{
    py::array_t<double> py_pmom(1 + nmom);
    double *ptr = static_cast<double *>(py_pmom.request().ptr);

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

    // set disort state
    //ds->nlyr = table["dim"]["nlyr"].value<int>().value_or(0);
    //ds->nmom = table["dim"]["nmom"].value<int>().value_or(0);
    //ds->nstr = table["dim"]["nstr"].value<int>().value_or(0);
    //ds->nphase = table["dim"]["nphase"].value<int>().value_or(0);

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
        ds->flag.prnt[i] =
            table["flag"]["prnt"][i].value<bool>().value_or(false);
    }

    ds->flag.usrtau = table["flag"]["usrtau"].value<bool>().value_or(false);
    /*if (ds->flag.usrtau) {
        ds->ntau = table["dim"]["ntau"].value<int>().value();
    }*/

    ds->flag.usrang = table["flag"]["usrang"].value<bool>().value_or(false);
    /*if (ds->flag.usrang) {
        ds->numu = table["dim"]["numu"].value<int>().value();
        ds->nphi = table["dim"]["nphi"].value<int>().value();
    } else {
        ds->nphi = 1;
    }*/

    //disort->Finalize();
    return disort;
}

DisortWrapper *DisortWrapper::SetAtmosphereDimension(
    int nlyr, int nmom, int nstr, int nphase) {
    if (_is_finalized) {
        // LOG(ERROR) << "Cannot set dimension after finalizing.";
        return this;
    }

    if (nlyr <= 0) {
        // LOG(ERROR) << "nlyr must be positive.";
        return this;
    }

    if (nmom <= 0) {
        // LOG(ERROR) << "nmom must be positive.";
        return this;
    }

    if (nstr <= 0) {
        // LOG(ERROR) << "nstr must be positive.";
        return this;
    }

    if (nphase <= 0) {
        // LOG(ERROR) << "nstr must be positive.";
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

DisortWrapper *DisortWrapper::SetIntensityDimension(int nphi, int numu,
                                                    int ntau) {
    if (_is_finalized) {
        // LOG(ERROR) << "Cannot set dimension after finalizing.";
        return this;
    }

    if (nphi <= 0) {
        // LOG(ERROR) << "nphi must be positive.";
        return this;
    }

    if (numu <= 0) {
        // LOG(ERROR) << "numu must be positive.";
        return this;
    }

    if (ntau <= 0) {
        // LOG(ERROR) << "ntau must be positive.";
        return this;
    }

    if (_ds.flag.usrang) {
        _ds.nphi = nphi;
        _ds.numu = numu;
    }

    if (_ds.flag.usrtau) _ds.ntau = ntau;
    return this;
}

void DisortWrapper::SetPhaseMoments(double *pmom, int nlyr, int nmom_p1)
{
    std::memcpy(_ds.pmom, pmom, nlyr * nmom_p1 * sizeof(double));
}

void DisortWrapper::runDisort() {
    if (!_is_finalized) {
        // LOG(ERROR) << "Disort is not finalized.";
        return;
    }

    // LOG(INFO) << "Set Disort boundary condition";
    _ds.bc.btemp = btemp;
    _ds.bc.ttemp = ttemp;
    _ds.bc.fluor = fluor;
    _ds.bc.albedo = albedo;
    _ds.bc.fisot = fisot;
    _ds.bc.fbeam = fbeam;
    _ds.bc.temis = temis;
    _ds.bc.umu0 = umu0;
    _ds.bc.phi0 = phi0;

    // LOG(INFO) << "Disort is running. ds = ";
    //printDisortState();
    c_disort(&_ds, &_ds_out);
    // LOG(INFO) << "Disort is finished. ds_out = ";
}

void DisortWrapper::printDisortState() {
    std::cout << "Leves = " << _ds.nlyr << std::endl;
    std::cout << "Moments = " << _ds.nmom << std::endl;
    std::cout << "Streams = " << _ds.nstr << std::endl;
    std::cout << "Phase functions = " << _ds.nphase << std::endl;
    std::cout << "Accuracy = " << _ds.accur << std::endl;
}
