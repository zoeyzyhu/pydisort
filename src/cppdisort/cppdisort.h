#ifndef DISORT_CPPDISORT_DISORTWRAPPER_H_
#define DISORT_CPPDISORT_DISORTWRAPPER_H_

#include <cdisort/cdisort.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <map>
#include <string>
#include <toml++/toml.h>
#include <tuple>

namespace py = pybind11;

// wraps c_getmom
py::array_t<double> getLegendreCoefficients(int nmom, std::string model, double gg = 0.);

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

    static DisortWrapper *FromFile(std::string_view filename) {
        return fromTomlTable(toml::parse_file(filename));
    }

    void SetHeader(std::string header);

    DisortWrapper *SetAtmosphereDimension(int nlyr, int nstr, int nmom, int nphase);

    DisortWrapper *SetFlags(std::map<std::string, bool> const &flags);

    DisortWrapper *SetIntensityDimension(int nuphi, int nutau, int numu);

    void Finalize() {
        if (!_is_finalized) {
            c_disort_state_alloc(&_ds);
            c_disort_out_alloc(&_ds, &_ds_out);
            _is_finalized = true;
        }
    }

    bool IsFinalized() const {
        return _is_finalized;
    }

    int nLayers() const {
        return _ds.nlyr;
    }

    int nMoments() const {
        return _ds.nmom;
    }

    int nStreams() const {
        return _ds.nstr;
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
        for (int i = 0; i < std::min(_ds.nlyr, len); ++i) {
            _ds.dtauc[i] = tau[i];
        }
        return this;
    }

    DisortWrapper *SetSingleScatteringAlbedo(double *ssa, int len) {
        for (int i = 0; i < std::min(_ds.nlyr, len); ++i) {
            _ds.ssalb[i] = ssa[i];
        }
        return this;
    }

    DisortWrapper *SetLevelTemperature(double *temp, int len) {
        // temperature array is defined on levels
        for (int i = 0; i <= std::min(_ds.nlyr, len - 1); ++i) {
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

    DisortWrapper *SetUserOpticalDepth(double *usrtau, int len) {
        if (_ds.flag.usrtau) {
            for (int i = 0; i < std::min(_ds.ntau, len); ++i) {
                _ds.utau[i] = usrtau[i];
            }
        }
        return this;
    }

    DisortWrapper *SetUserCosinePolarAngle(double *umu, int len) {
        if (_ds.flag.usrang) {
            for (int i = 0; i < std::min(_ds.numu, len); ++i) {
                _ds.umu[i] = umu[i];
            }
        }
        return this;
    }

    DisortWrapper *SetUserAzimuthalAngle(double *phi, int len) {
        if (_ds.flag.usrang) {
            for (int i = 0; i < std::min(_ds.nphi, len); ++i) {
                _ds.phi[i] = phi[i];
            }
        }
        return this;
    }

    void SetPlanckSource(double *planck);

    // pmom is a 1D array of length nlyr * (nmom + 1)
    // with nlyr being the number of layers and nmom the number of scattering moments
    void SetPhaseMoments(double *pmom, int nlyr, int nmom_p1);

    //std::tuple<std::vector<double>, std::vector<double>> RunRTFlux() {
    py::array_t<double> RunRTFlux(std::string outputs) {
        runDisort();

        std::vector<std::string> allfields = {
            "rfldir", "rfldn", "flup", "dfdt", "uavg", "uavgdn",
            "uavgup", "uavgso"};

        //for (int i = 0; i < _ds.nlyr; ++i) {
        //    flxup[i] = _ds_out.rad[i].flup;
        //    flxdn[i] = _ds_out.rad[i].rfldir + _ds_out.rad[i].rfldn;
        //}

        //py::array_t<double> ndarray({_ds.nlyr + 1}, _ds_out.rad);
        py::array_t<double> ndarray;

        return ndarray;
   }

   py::array_t<double> RunRTIntensity() {
       runDisort();
       py::array_t<double> ndarray(
            {_ds.nphi, _ds.ntau, _ds.numu}, _ds_out.uu
            );
       return ndarray;
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
    void printDisortFlags();
};

// exposing private members for testing
class DisortWrapperTestOnly : public DisortWrapper {
   public:
    static DisortWrapper *FromString(std::string_view content) {
        return fromTomlTable(toml::parse(content));
    }

    disort_state *GetDisortState() { return &_ds; }
    disort_output *GetDisortOutput() { return &_ds_out; }
};

#endif  // DISORT_CPPDISORT_DISORTWRAPPER_H_
