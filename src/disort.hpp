#pragma once

// C/C++
#include <map>
#include <string>
#include <utility>
#include <vector>

// torch
#include <torch/nn/cloneable.h>
#include <torch/nn/functional.h>
#include <torch/nn/module.h>
#include <torch/nn/modules/common.h>
#include <torch/nn/modules/container/any.h>

// disort
#include <cdisort213/cdisort.h>

#include "add_arg.h"

namespace disort {

DISPATCH_MACRO
inline double emission_temp(double wlo, double whi, double temp) {
  return temp;
}

struct DisortOptions {
  DisortOptions();

  //! set disort header
  void set_header(std::string const& header);

  //! set disort flags
  void set_flags(std::string const& flags);

  //! emission function
  ADD_ARG(std::function<double(double, double, double)>,
          emission) = emission_temp;

  //! header
  ADD_ARG(std::string, header) = "running disort ...";
  ADD_ARG(std::string, flags) = "";

  //! spectral dimensions
  ADD_ARG(int, nwave) = 1;

  //! spatial dimensions
  ADD_ARG(int, ncol) = 1;

  //! user optical depth grid
  ADD_ARG(std::vector<double>, user_tau) = { 0. };

  //! user polar angle grid
  ADD_ARG(std::vector<double>, user_mu) = { 1. };

  //! user azimuthal angle grid
  ADD_ARG(std::vector<double>, user_phi) = { 0. };

  //! set lower wavenumber(length) at each bin
  ADD_ARG(std::vector<double>, wave_lower) = {};

  //! set upper wavenumber(length) at each bin
  ADD_ARG(std::vector<double>, wave_upper) = {};

  //! placeholder for disort state
  ADD_ARG(disort_state, ds);
};

class DisortImpl : public torch::nn::Cloneable<DisortImpl> {
 public:
  //! options with which this `DisortImpl` was constructed
  DisortOptions options;

  //! Constructor to initialize the layers
  DisortImpl() = default;
  explicit DisortImpl(DisortOptions const& options);
  virtual ~DisortImpl();
  void reset() override;
  void pretty_print(std::ostream& stream) const override;

  //! disort state at one wave and one column
  /*!
   * \param n wave index
   * \param j column index
   * \return disort state
   */
  disort_state const& ds(int n = 0, int j = 0) const {
    return ds_[n * options.ncol() + j];
  }

  //! disort state at one wave and one column
  /*!
   * \param n wave index
   * \param j column index
   * \return disort state
   */
  disort_state& ds(int n = 0, int j = 0) { return ds_[n * options.ncol() + j]; }

  //! disort output at one wave and one column
  /*!
   * \param n wave index
   * \param j column index
   * \return disort output
   */
  disort_output const& ds_out(int n = 0, int j = 0) const {
    return ds_out_[n * options.ncol() + j];
  }

  //! disort output at one wave and one column
  /*!
   * \param n wave index
   * \param j column index
   * \return disort output
   */
  disort_output& ds_out(int n = 0, int j = 0) {
    return ds_out_[n * options.ncol() + j];
  }

  //! disort flux outputs
  /*!
   * Disort outputs the following 8 flux variables:
   * 0 : direct beam flux (rfldir)
   * 1 : diffuse downward flux (fldn)
   * 2 : diffuse upward flux (flup)
   * 3 : flux divergence, d (net flux) / d (optical depth) (rfldn)
   * 4 : mean intensity including direct beam (uavg)
   * 5 : mean diffuse downward intensity (uavgdn)
   * 6 : mean diffuse upward intensity (uavgup)
   * 7 : mean direct beam (uavgso)
   *
   * \param op tensor options
   * \return disort flux outputs (nwave, ncol, nlvl = nlyr + 1, 8)
   */
  torch::Tensor get_flx(torch::TensorOptions op) const;

  //! disort radiance outputs
  /*!
   * \param op tensor options
   * \return disort radiance outputs (nwave, ncol, nphi, ntau, numu)
   */
  torch::Tensor get_rad(torch::TensorOptions op) const;

  //! Calculate radiative flux or intensity
  /*!
   * \param prop properties at each level (nwave, ncol, nlyr, nprop)
   * \param bc dictionary of disort boundary conditions each of size (nwave,
   * ncol)
   * \param temf temperature at each level (ncol, nlvl = nlyr + 1)
   * \return radiative flux or intensity (nwave, ncol, nlvl, 2)
   */
  torch::Tensor forward(torch::Tensor prop,
                        std::map<std::string, torch::Tensor>* bc,
                        torch::optional<torch::Tensor> temf = torch::nullopt);

 private:
  //! flat array of disort states (nwave * ncol)
  std::vector<disort_state> ds_;

  //! flat array of disort outputs (nwave * ncol)
  std::vector<disort_output> ds_out_;

  //! flag to indicate if disort memory has been allocated
  bool allocated_ = false;
};
TORCH_MODULE(Disort);

//! Print disort flags
void print_ds_flags(std::ostream& os, disort_state const& ds);

//! Print disort atmosphere dimensions
void print_ds_atm(std::ostream& os, disort_state const& ds);

//! Print disort output dimensions
void print_ds_out(std::ostream& os, disort_state const& ds);

//! Print disort boundary conditions
void print_ds_bc(std::ostream& os, disort_state const& ds);

}  // namespace disort
