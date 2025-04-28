#pragma once

// torch
#include <torch/torch.h>

namespace disort {

//! Compute the scattering phase moment
/*!
 * \param npmom Number of phase moments
 * \param type Phase function type, choose from
 *             - "isotropic"
 *             - "rayleigh"
 *             - "henyey-greenstein"
 *             - "double-henyey-greenstein"
 *             - "haze-garcia-siewert"
 *             - "cloud-garcia-siewert"
 * \param gg1 First Henyey-Greenstein parameter
 * \param gg2 Second Henyey-Greenstein parameter
 * \param ff Weight of the first Henyey-Greenstein parameter
 * \return 1D tensor of phase moments, size = (npmom,)
 */
torch::Tensor scattering_moments(int npmom,
                                 std::string const &type = "isotropic",
                                 double gg1 = 0., double gg2 = 0.,
                                 double ff = 0.);

}  // namespace disort
