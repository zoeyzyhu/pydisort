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

torch::Tensor scattering_moments(int npmom, PhaseMomentOptions const &options) {
  torch::Tensor pmom = torch::zeros({1 + npmom}, torch::kDouble);

  if (options.type() == kHenyeyGreenstein) {
    TORCH_CHECK(options.gg() > -1. && options.gg() < 1.,
                "scattering_moments::bad input variable gg");
    for (int k = 1; k <= npmom; k++) {
      pmom[k] = pow(options.gg(), k);
    }
  } else if (options.type() == kDoubleHenyeyGreenstein) {
    auto gg1 = options.gg1();
    auto gg2 = options.gg2();
    auto ff = options.ff();

    TORCH_CHECK(gg1 > -1. && gg1 < 1. && gg2 > -1. && gg2 < 1.,
                "scattering_moments::bad input variable gg1 or gg2");

    for (int k = 1; k <= npmom; k++) {
      pmom[k] = ff * pow(gg1, k) + (1. - ff) * pow(gg2, k);
    }
  } else if (options.type() == kRayleigh) {
    TORCH_CHECK(npmom >= 2, "scattering_moments::npmom < 2");
    pmom[2] = 0.1;
  } else if (options.type() == kIsotropic) {
    // nothing to do
  } else if (options.type() == kHazeGarciaSiewert) {
    c_getmom(HAZE_GARCIA_SIEWERT, options.gg(), npmom, pmom.data_ptr<double>());
  } else if (options.type() == kCloudGarciaSiewert) {
    c_getmom(CLOUD_GARCIA_SIEWERT, options.gg(), npmom,
             pmom.data_ptr<double>());
  } else {
    throw std::runtime_error("scattering_moment::unknown phase function");
  }

  return pmom.narrow(0, 1, npmom);
}

}  // namespace disort
