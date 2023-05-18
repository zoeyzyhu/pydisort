#ifndef PROD_RAD_DISORT_DISORT_WRAPPER_H_
#define PROD_RAD_DISORT_DISORT_WRAPPER_H_

#include <cdisort/cdisort.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <map>
#include <string>
#include <toml++/toml.h>
#include <tuple>

namespace py = pybind11;

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

    static DisortWrapper *FromFile(std::string_view filename) {
        return fromTomlTable(toml::parse_file(filename));
    }

    DisortWrapper *SetAtmosphereDimension(int nlyr, int nstr, int nmom,
                                          int nphase);

    DisortWrapper *SetFlags(std::map<std::string, bool> const &flags);

    DisortWrapper *SetIntensityDimension(int nphi, int ntau, int numu);

    void Finalize() {
        if (_is_finalized) {
            throw std::runtime_error("Disort is already initialized!");
        }

        c_disort_state_alloc(&_ds);
        c_disort_out_alloc(&_ds, &_ds_out);

        _is_finalized = true;
    }

    void SetAccuracy(double accur) { _ds.accur = accur; }

    void SetFlags(py::dict const& dict);

    virtual ~DisortWrapper() {
        if (_is_finalized) {
            c_disort_state_free(&_ds);
            c_disort_out_free(&_ds, &_ds_out);
            _is_finalized = false;
        }
    }

    DisortWrapper *SetOpticalDepth(double *tau, int len) {
        if (len != _ds.nlyr) {
            throw std::runtime_error("Optical depth array length mismatch!");
        }
        for (int i = 0; i < _ds.nlyr; ++i) {
            _ds.dtauc[i] = tau[i];
        }
        return this;
    }

    DisortWrapper *SetSingleScatteringAlbedo(double *ssa, int len) {
        if (len != _ds.nlyr) {
            throw std::runtime_error(
                "Single Scattering array length mismatch!");
        }

        for (int i = 0; i < _ds.nlyr; ++i) {
            _ds.ssalb[i] = ssa[i];
        }
        return this;
    }

    DisortWrapper *SetLevelTemperature(double *temp, int len) {
        if (len != _ds.nlyr + 1) {
            throw std::runtime_error("Temperature array length mismatch!");
        }
        for (int i = 0; i <= _ds.nlyr; ++i) {
            if (temp[i] < 0) {
                throw std::runtime_error("Temperature must be positive!");
            }
            _ds.temper[i] = temp[i];
        }
        return this;
    }

    DisortWrapper *SetWavenumberRange_invcm(double wmin, double wmax) {
        _ds.wvnmlo = wmin;
        _ds.wvnmhi = wmax;
        return this;
    }

    DisortWrapper *SetWavenumber_invcm(double wave) {
        _ds.wvnmlo = wave;
        _ds.wvnmhi = wave;
        return this;
    }

    DisortWrapper *SetOutputOpticalDepth(double *usrtau, int len) {
        if (len != _ds.ntau) {
            throw std::runtime_error(
                "Output optical depth array length mismatch!");
        }

        if (_ds.flag.usrtau) {
            for (int i = 0; i < _ds.ntau; ++i) {
                _ds.utau[i] = usrtau[i];
            }
        }
        return this;
    }

    DisortWrapper *SetOutgoingRay(double *umu, double *phi) {
        if (_ds.flag.usrang) {
            for (int i = 0; i < _ds.numu; ++i) {
                _ds.umu[i] = umu[i];
            }
            for (int i = 0; i < _ds.nphi; ++i) {
                _ds.phi[i] = phi[i];
            }
        }
        return this;
    }

    void SetPlanckSource(double *planck);

    void SetLegendreCoefficients(double **legendre);

    std::tuple<std::vector<double>, std::vector<double>> RunRTFlux() {
        _ds.flag.onlyfl = true;
        runDisort();
        std::vector<double> flxup(_ds.nlyr);
        std::vector<double> flxdn(_ds.nlyr);

        for (int i = 0; i < _ds.nlyr; ++i) {
            flxup[i] = _ds_out.rad[i].flup;
            flxdn[i] = _ds_out.rad[i].rfldir + _ds_out.rad[i].rfldn;
        }

        return std::make_tuple(flxup, flxdn);
    }

   py::array_t<double> RunRTIntensity() {
       _ds.flag.onlyfl = false;
       runDisort();
       py::array_t<double> numpy_array({_ds.nphi, _ds.ntau, _ds.numu},
                                       _ds_out.uu);
       return numpy_array;
   }

   protected:
    disort_state _ds;
    disort_output _ds_out;

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
        _ds.accur = 1.E-6;
    }

    bool _is_finalized = false;
    static DisortWrapper *fromTomlTable(const toml::table &table);

    void runDisort();
    void printDisortState();
};

class DisortWrapperTestOnly : public DisortWrapper {
   public:
    static DisortWrapper *FromString(std::string_view content) {
        return fromTomlTable(toml::parse(content));
    }

    disort_state *GetDisortState() { return &_ds; }
    disort_output *GetDisortOutput() { return &_ds_out; }
};

#endif  // PROD_RAD_DISORT_DISORT_WRAPPER_H_
