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

    static DisortWrapper *FromFile(std::string_view filename) {
        return fromTomlTable(toml::parse_file(filename));
    }

    virtual ~DisortWrapper();

    void SetHeader(std::string header);

    DisortWrapper *SetAtmosphereDimension(int nlyr, int nstr, int nmom, int nphase);

    DisortWrapper *SetFlags(std::map<std::string, bool> const &flags);

    DisortWrapper *SetIntensityDimension(int nuphi, int nutau, int numu);

    //void SetFlags(py::dict const& dict);

    void Finalize();

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

    void SetAccuracy(double accur) {
        _ds.accur = accur;
    }

    void SetWavenumberRange_invcm(double wmin, double wmax) {
        _ds.wvnmlo = wmin;
        _ds.wvnmhi = wmax;
    }

    void SetWavenumber_invcm(double wave) {
        _ds.wvnmlo = wave;
        _ds.wvnmhi = wave;
    }

    void SetOpticalDepth(double *tau, int len);

    void SetSingleScatteringAlbedo(double *ssa, int len);

    // temperature array is defined on levels
    void SetLevelTemperature(double *temp, int len);

    void SetUserOpticalDepth(double *usrtau, int len);

    void SetUserCosinePolarAngle(double *umu, int len);

    void SetUserAzimuthalAngle(double *phi, int len);

    void SetPlanckSource(double *planck);

    // pmom is a 1D array of length nlyr * (nmom + 1)
    // with nlyr being the number of layers and nmom the number of scattering moments
    void SetPhaseMoments(double *pmom, int nlyr, int nmom_p1);

    DisortWrapper* Run();
    // \todo how to make them actually constant ?
    py::array_t<double> GetFlux() const;
    py::array_t<double> GetIntensity() const;

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
