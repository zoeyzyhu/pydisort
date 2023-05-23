#include <iostream>
#include <stdexcept>
#include <string>
#include <cppdisort/cppdisort.h>

template<typename A>
void assert_eq(std::string msg, A value1, A value2) {
    if (value1 != value2) {
        std::cerr << "- Failing \'" << msg << "\', "
                  << "Expect " << value2 << ", Got " << value1 << std::endl;
    } else {
        std::cout << "- PASS " << msg << std::endl;
    }
}

int main() {
    // create an instance of DisortWrapper
    DisortWrapper* disort = DisortWrapper::FromFile("input.toml");

    // disort wrapper should be finalzied now
    assert_eq("Disort finalize", disort->IsFinalized(), true);
    assert_eq("Atmosphere layer", disort->nLayers(), 5);
    assert_eq("Radiation streams", disort->nStreams(), 4);
    assert_eq("phase function moments", disort->nMoments(), 4);

    //disort->SetFlags(flags);

    // Set intensity dimensions
    disort->SetIntensityDimension(10, 10, 10);

    // Set other parameters and data

    // Finalize the DisortWrapper
    disort->Finalize();

    // Run the DisortWrapper and get the results
    //std::tuple<std::vector<double>, std::vector<double>> fluxes = disort->RunRTFlux();

    /* Print the results
    std::vector<double> flxup = std::get<0>(fluxes);
    std::vector<double> flxdn = std::get<1>(fluxes);

    std::cout << "Flux Up:" << std::endl;
    for (const double& flux : flxup) {
        std::cout << flux << " ";
    }
    std::cout << std::endl;

    std::cout << "Flux Down:" << std::endl;
    for (const double& flux : flxdn) {
        std::cout << flux << " ";
    }
    std::cout << std::endl;*/

    py::array_t<double> intensities = disort->Run()->GetIntensity();

    // Access intensity data
    py::buffer_info info = intensities.request();
    double* data = static_cast<double*>(info.ptr);
    std::cout << "Intensity Data:" << std::endl;
    for (int i = 0; i < info.size; ++i) {
        std::cout << data[i] << " ";
    }
    std::cout << std::endl;

    // Cleanup
    delete disort;

    return 0;
}
