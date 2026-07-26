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
#include <cdisort213/cdisort.hpp>

#include "add_arg.h"

namespace disort {

struct DisortOptionsImpl {
  DisortOptionsImpl();
  static std::shared_ptr<DisortOptionsImpl> create() {
    return std::make_shared<DisortOptionsImpl>();
  }

  std::shared_ptr<DisortOptionsImpl> clone() const {
    return std::make_shared<DisortOptionsImpl>(*this);
  }
  void report(std::ostream& os) const {
    os << "* header = " << header() << "\n"
       << "* flags = " << flags() << "\n"
       << "* nwave = " << nwave() << "\n"
       << "* ncol = " << ncol() << "\n"
       << "* accur = " << accur() << "\n"
       << "* upward = " << upward() << "\n";

    os << "* user_tau = ";
    for (auto const& v : user_tau()) os << v << ", ";
    os << "\n";

    os << "* user_mu = ";
    for (auto const& v : user_mu()) os << v << ", ";
    os << "\n";

    os << "* user_phi = ";
    for (auto const& v : user_phi()) os << v << ", ";
    os << "\n";

    os << "* wave_lower = ";
    for (auto const& v : wave_lower()) os << v << ", ";
    os << "\n";

    os << "* wave_upper = ";
    for (auto const& v : wave_upper()) os << v << ", ";
    os << "\n";
  }

  //! set disort header
  void set_header(std::string const& header);

  //! set disort flags
  void set_flags(std::string const& flags);

  //! emission function
  ADD_ARG(std::function<double(double, double, double)>,
          emission) = c_planck_func2;

  //! header
  ADD_ARG(std::string, header) = "running disort ...";
  ADD_ARG(std::string, flags) = "";

  //! spectral dimensions
  ADD_ARG(int, nwave) = 1;

  //! spatial dimensions
  ADD_ARG(int, ncol) = 1;

  //! accuracy
  ADD_ARG(double, accur) = 1.e-6;

  //! direction
  /*!
   * 0 : downward (disort default)
   * 1 : upward (atmosphere radiative transfer)
   */
  ADD_ARG(int, upward) = 0;

  //! user optical depth grid
  ADD_ARG(std::vector<double>, user_tau) = {0.};

  //! user polar angle grid
  ADD_ARG(std::vector<double>, user_mu) = {1.};

  //! user azimuthal angle grid
  ADD_ARG(std::vector<double>, user_phi) = {0.};

  //! set lower wavenumber(length) at each bin
  ADD_ARG(std::vector<double>, wave_lower) = {};

  //! set upper wavenumber(length) at each bin
  ADD_ARG(std::vector<double>, wave_upper) = {};

  //! placeholder for disort state
  ADD_ARG(disort_state, ds);
};

//! Shared pointer to DisortOptionsImpl
/*!
 * DisortOptions is designed to be shared across multiple DisortImpl instances.
 * This allows multiple instances to share the same configuration and modify
 * it collectively. When the options are modified (e.g., via reset()), all
 * instances sharing the same DisortOptions object will see the changes.
 */
using DisortOptions = std::shared_ptr<DisortOptionsImpl>;

class DisortImpl : public torch::nn::Cloneable<DisortImpl> {
 public:
  //! options with which this `DisortImpl` was constructed
  DisortOptions options;

  //! Constructor to initialize the layers
  DisortImpl() : options(DisortOptionsImpl::create()) {}
  
  //! Constructor with shared options
  /*!
   * Constructs a DisortImpl instance with the provided shared options.
   * Multiple DisortImpl instances can share the same DisortOptions object,
   * allowing them to share configuration and see modifications made by any
   * instance via reset() or other methods.
   * 
   * \param options Shared pointer to DisortOptionsImpl to be used by this instance
   */
  explicit DisortImpl(DisortOptions const& options);
  virtual ~DisortImpl();
  
  //! Reset and reinitialize the disort state
  /*!
   * This method modifies the shared options object. If multiple DisortImpl
   * instances share the same DisortOptions, all instances will be affected
   * by this reset operation.
   */
  void reset() override;
  void pretty_print(std::ostream& stream) const override;

  //! disort state at one wave and one column
  /*!
   * \param n wave index
   * \param j column index
   * \return disort state
   */
  disort_state const& ds(int n = 0, int j = 0) const {
    return ds_[n * options->ncol() + j];
  }

  //! disort state at one wave and one column
  /*!
   * \param n wave index
   * \param j column index
   * \return disort state
   */
  disort_state& ds(int n = 0, int j = 0) { return ds_[n * options->ncol() + j]; }

  //! disort output at one wave and one column
  /*!
   * \param n wave index
   * \param j column index
   * \return disort output
   */
  disort_output const& ds_out(int n = 0, int j = 0) const {
    return ds_out_[n * options->ncol() + j];
  }

  //! disort output at one wave and one column
  /*!
   * \param n wave index
   * \param j column index
   * \return disort output
   */
  disort_output& ds_out(int n = 0, int j = 0) {
    return ds_out_[n * options->ncol() + j];
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
  torch::Tensor gather_flx() const;

  //! disort radiance outputs
  /*!
   * \param op tensor options
   * \return disort radiance outputs (nwave, ncol, nphi, ntau, numu)
   */
  torch::Tensor gather_rad() const;

  //! Calculate radiative flux or intensity
  /*!
   * \param prop optical properties at each level (nwave, ncol, nlyr, nprop)
   *
   * \param bc dictionary of disort boundary conditions
   *        The dimensions of each recognized key are:
   *        - <band> + "umu0" : (ncol,), cosine of solar zenith angle
   *        - <band> + "phi0" : (ncol,), azimuthal angle of solar beam
   *        - <band> + "fbeam" : (nwave, ncol), solar beam flux
   *        - <band> + "albedo" : (nwave, ncol), surface albedo
   *        - <band> + "fluor" : (nwave, ncol), isotropic bottom illumination
   *        - <band> + "fisot" : (nwave, ncol), isotropic top illumination
   *        - <band> + "temis" : (nwave, ncol), top emissivity
   *        - "btemp" : (ncol,), bottom temperature
   *        - "ttemp" : (ncol,), top temperature
   *
   *        Some keys can have a prefix band name, <band>.
   *        If the prefix is an non-empty string, a slash "/" is
   *        automatically appended to it, such that the key look like
   *        `B1/umu0`. `btemp` and `ttemp` do not have a band name prefix.
   *
   * \param bname name of the radiation band
   * \param temf temperature at each level (ncol, nlvl = nlyr + 1)
   * \return radiative flux or intensity (nwave, ncol, nlvl, nrad)
   */
  torch::Tensor forward(torch::Tensor prop,
                        std::map<std::string, torch::Tensor>* bc,
                        std::string bname = "",
                        torch::optional<torch::Tensor> temf = torch::nullopt);

  //! Release the cached CUDA workspace associated with this solver instance.
  void release_cuda_workspace() { cuda_workspace_ = torch::Tensor(); }

 protected:
  // This allows type erasure with default arguments
  FORWARD_HAS_DEFAULT_ARGS({2, torch::nn::AnyValue("")},
                           {3, torch::nn::AnyValue(torch::nullopt)})

 private:
  //! Reused CUDA scratch storage; ignored by the CPU dispatch path.
  torch::Tensor cuda_workspace_;

  //! flat array of disort states (nwave * ncol)
  std::vector<disort_state> ds_;

  //! flat array of disort outputs (nwave * ncol)
  std::vector<disort_output> ds_out_;

  //! tensor output options after running disort
  torch::TensorOptions result_options_;

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

#undef ADD_ARG
