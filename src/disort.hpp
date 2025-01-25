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

struct DisortOptions {
  DisortOptions();
  void set_header(std::string const& header);
  void set_flags(std::string const& flags);

  // header
  ADD_ARG(std::string, header) = "running disort ...";
  ADD_ARG(std::string, flags) = "";

  // spectral dimensions
  ADD_ARG(int, nwave) = 1;

  // spatial dimensions
  ADD_ARG(int, ncol) = 1;

  // user optical depth grid
  ADD_ARG(std::vector<double>, user_tau) = { 0. };

  // user polar angle grid
  ADD_ARG(std::vector<double>, user_mu) = { 1. };

  // user azimuthal angle grid
  ADD_ARG(std::vector<double>, user_phi) = { 0. };

  // placeholder for disort state
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

  disort_state const& ds(int n = 0, int j = 0) const {
    return ds_[n * options.ncol() + j];
  }
  disort_output const& ds_out(int n = 0, int j = 0) const {
    return ds_out_[n * options.ncol() + j];
  }

  disort_state& ds(int n = 0, int j = 0) { return ds_[n * options.ncol() + j]; }
  disort_output& ds_out(int n = 0, int j = 0) {
    return ds_out_[n * options.ncol() + j];
  }

  torch::Tensor get_flx(torch::TensorOptions op) const;
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
                        std::map<std::string, torch::Tensor>& bc,
                        torch::optional<torch::Tensor> temf = torch::nullopt);

 private:
  std::vector<disort_state> ds_;
  std::vector<disort_output> ds_out_;
  bool allocated_ = false;
};
TORCH_MODULE(Disort);

void print_ds_flags(std::ostream& os, disort_state const& ds);
void print_ds_atm(std::ostream& os, disort_state const& ds);
void print_ds_out(std::ostream& os, disort_state const& ds);
void print_ds_bc(std::ostream& os, disort_state const& ds);

}  // namespace disort
