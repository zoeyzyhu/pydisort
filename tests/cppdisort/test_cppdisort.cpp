// C/C++
#include <cassert>
#include <iostream>

// pydisort
#include <cppdisort/cppdisort.hpp>

int main() {
  // Test case for DisortWrapper::FromFile method
  DisortWrapper *disort = DisortWrapper::FromFile("isotropic_scattering.toml");
  assert(disort != nullptr);

  // Test case for DisortWrapper::SetHeader method
  disort->SetHeader("Test header");

  // Test case for DisortWrapper::SetAtmosphereDimension method
  disort->SetAtmosphereDimension(3, 4, 5, 6);

  // Test case for DisortWrapper::SetFlags method
  std::map<std::string, bool> flags = {{"ibcnd", true},
                                       {"usrtau", false},
                                       {"usrang", true},
                                       {"lamber", false},
                                       {"planck", true},
                                       {"spher", false},
                                       {"onlyfl", true},
                                       {"quiet", false},
                                       {"intensity_correction", true},
                                       {"old_intensity_correction", false},
                                       {"general_source", true},
                                       {"output_uum", false}};
  disort->SetFlags(flags);

  // Test case for DisortWrapper::SetIntensityDimension method
  disort->SetIntensityDimension(7, 8, 9);

  // Test case for DisortWrapper::Finalize method
  disort->Finalize();

  // Test case for DisortWrapper::IsFinalized method
  assert(disort->IsFinalized());

  // Test case for DisortWrapper::nLayers method
  int numLayers = disort->nLayers();
  std::cout << "Number of layers: " << numLayers << std::endl;

  // Test case for DisortWrapper::nMoments method
  int numMoments = disort->nMoments();
  std::cout << "Number of moments: " << numMoments << std::endl;

  // Test case for DisortWrapper::nStreams method
  int numStreams = disort->nStreams();
  std::cout << "Number of streams: " << numStreams << std::endl;

  delete disort;

  return 0;
}
