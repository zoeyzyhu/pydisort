#include <iostream>
#include <cppdisort/cppdisort.h>


int main() {
    // Create an instance of DisortWrapper
    DisortWrapper* disort = DisortWrapper::FromFile("input.toml");

    // Set atmosphere dimensions
    disort->SetAtmosphereDimension(10, 10, 10, 10);

    // Set flags
    std::map<std::string, bool> flags = {
        {"ibcnd", true},
        {"usrtau", true},
        {"usrang", true},
        // ... set other flags as needed
    };
    disort->SetFlags(flags);

    // Set intensity dimensions
    disort->SetIntensityDimension(10, 10, 10);

    // Set other parameters and data

    // Finalize the DisortWrapper
    disort->Finalize();

    // Run the DisortWrapper and get the results
    std::tuple<std::vector<double>, std::vector<double>> fluxes = disort->RunRTFlux();
    py::array_t<double> intensities = disort->RunRTIntensity();

    // Print the results
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
    std::cout << std::endl;

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

