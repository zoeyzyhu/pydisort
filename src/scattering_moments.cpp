// C/C++
#include <cmath>

// cdisort
#include <cdisort213/cdisort.h>  // c_getmom

// conflict with torch
#undef A
#undef B

// disort
#include "scattering_moments.hpp"

namespace disort {

torch::Tensor scattering_moments(int npmom, std::string const &type, double gg1,
                                 double gg2, double ff) {
  torch::Tensor pmom = torch::zeros({1 + npmom}, torch::kDouble);
  pmom[0] = 1.0;

  if (type == "henyey-greenstein") {
    TORCH_CHECK(gg1 > -1. && gg1 < 1.,
                "scattering_moments::bad input variable gg");
    for (int k = 1; k <= npmom; k++) {
      pmom[k] = pow(gg1, k);
    }
  } else if (type == "double-henyey-greenstein") {
    TORCH_CHECK(gg1 > -1. && gg1 < 1. && gg2 > -1. && gg2 < 1.,
                "scattering_moments::bad input variable gg1 or gg2");

    for (int k = 1; k <= npmom; k++) {
      pmom[k] = ff * pow(gg1, k) + (1. - ff) * pow(gg2, k);
    }
  } else if (type == "rayleigh") {
    TORCH_CHECK(npmom >= 2, "scattering_moments::npmom < 2");
    pmom[2] = 0.1;
  } else if (type == "isotropic") {
    // nothing to do
  } else if (type == "haze-garcia-siewert") {
    c_getmom(HAZE_GARCIA_SIEWERT, gg1, npmom, pmom.data_ptr<double>());
  } else if (type == "cloud-garcia-siewert") {
    c_getmom(CLOUD_GARCIA_SIEWERT, gg1, npmom, pmom.data_ptr<double>());
  } else {
    TORCH_CHECK(false, "scattering_moments::unknown phase function");
  }

  return pmom.narrow(0, 1, npmom);
}

}  // namespace disort
