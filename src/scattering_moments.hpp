#pragma once

// torch
#include <torch/torch.h>

#include "add_arg.h"

namespace disort {

enum {
  // phase functions
  kIsotropic = 0,
  kRayleigh = 1,
  kHenyeyGreenstein = 2,
  kDoubleHenyeyGreenstein = 3,
  kHazeGarciaSiewert = 4,
  kCloudGarciaSiewert = 5,
};

struct PhaseMomentOptions {
  ADD_ARG(int, type) = kRayleigh;
  ADD_ARG(double, gg) = 0.;
  ADD_ARG(double, gg1) = 0.;
  ADD_ARG(double, gg2) = 0.;
  ADD_ARG(double, ff) = 0.;
};

//! Compute the scattering phase moment
/*!
 * \param npmom Number of phase moments
 * \param options Options for the phase function
 * \return 1D tensor of phase moments, size = (npmom,)
 */
torch::Tensor scattering_moments(int npmom, PhaseMomentOptions const &options);

}  // namespace disort
