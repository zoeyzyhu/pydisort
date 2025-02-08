// C/C++
#include <map>

// torch
#include <ATen/TensorIterator.h>

// disort
#include "disort.hpp"
#include "disort_formatter.hpp"
#include "vectorize.hpp"

namespace disort {

void call_disort_cpu(at::TensorIterator& iter, int rank_in_column,
                     disort_state* ds, disort_output* ds_out);

void call_disort_cuda(at::TensorIterator& iter, int rank_in_column,
                      disort_state* ds, disort_output* ds_out);

DisortOptions::DisortOptions() {
  // flags
  ds().flag.ibcnd = false;
  ds().flag.usrtau = false;
  ds().flag.usrang = false;
  ds().flag.lamber = false;
  ds().flag.planck = false;
  ds().flag.spher = false;
  ds().flag.onlyfl = false;
  ds().flag.quiet = false;
  ds().flag.intensity_correction = false;
  ds().flag.old_intensity_correction = false;
  ds().flag.general_source = false;
  ds().flag.output_uum = false;
  for (int i = 0; i < 5; ++i) {
    ds().flag.prnt[i] = false;
  }

  // bc
  ds().bc.btemp = 0.;
  ds().bc.ttemp = 0.;
  ds().bc.fluor = 0.;
  ds().bc.albedo = 0.;
  ds().bc.fisot = 0.;
  ds().bc.fbeam = 0.;
  ds().bc.temis = 0.;
  ds().bc.umu0 = 1.;
  ds().bc.phi0 = 0.;
  ds().accur = 1.E-6;
}

void DisortOptions::set_header(std::string const& header) {
  snprintf(ds().header, sizeof(ds().header), "%s", header.c_str());
}

void DisortOptions::set_flags(std::string const& str) {
  std::vector<std::string> dstr = Vectorize<std::string>(str.c_str(), " ,");

  for (int i = 0; i < dstr.size(); ++i) {
    if (dstr[i] == "ibcnd") {
      ds().flag.ibcnd = true;
    } else if (dstr[i] == "usrtau") {
      ds().flag.usrtau = true;
    } else if (dstr[i] == "usrang") {
      ds().flag.usrang = true;
    } else if (dstr[i] == "lamber") {
      ds().flag.lamber = true;
    } else if (dstr[i] == "planck") {
      ds().flag.planck = true;
    } else if (dstr[i] == "spher") {
      ds().flag.spher = true;
    } else if (dstr[i] == "onlyfl") {
      ds().flag.onlyfl = true;
    } else if (dstr[i] == "quiet") {
      ds().flag.quiet = true;
    } else if (dstr[i] == "intensity_correction") {
      ds().flag.intensity_correction = true;
    } else if (dstr[i] == "old_intensity_correction") {
      ds().flag.old_intensity_correction = true;
    } else if (dstr[i] == "general_source") {
      ds().flag.general_source = true;
    } else if (dstr[i] == "output_uum") {
      ds().flag.output_uum = true;
    } else if (dstr[i] == "print-input") {
      ds().flag.prnt[0] = true;
    } else if (dstr[i] == "print-fluxes") {
      ds().flag.prnt[1] = true;
    } else if (dstr[i] == "print-intensity") {
      ds().flag.prnt[2] = true;
    } else if (dstr[i] == "print-transmissivity") {
      ds().flag.prnt[3] = true;
    } else if (dstr[i] == "print-phase-function") {
      ds().flag.prnt[4] = true;
    } else {
      std::stringstream msg;
      msg << "flag: '" << dstr[i] << "' unrecognized" << std::endl;
      throw std::runtime_error("DisortOptions::set_flags::" + msg.str());
    }
  }
}

DisortImpl::DisortImpl(DisortOptions const& options_) : options(options_) {
  reset();
}

void DisortImpl::reset() {
  options.set_header(options.header());
  options.set_flags(options.flags());

  options.ds().nphi = options.user_phi().size();
  options.ds().numu = options.user_mu().size();
  options.ds().ntau = options.user_tau().size();

  TORCH_CHECK(options.ds().nlyr > 0, "DisortImpl: ds.nlyr <= 0");
  TORCH_CHECK(options.ds().nstr > 0, "DisortImpl: ds.nstr <= 0");
  TORCH_CHECK(options.ds().nmom >= options.ds().nstr,
              "DisortImpl: ds.nmom < ds.nstr");

  if (options.ds().flag.planck) {
    TORCH_CHECK(options.wave_lower().size() == options.nwave(),
                "DisortImpl: wave_lower.size() != nwave");
    TORCH_CHECK(options.wave_upper().size() == options.nwave(),
                "DisortImpl: wave_upper.size() != nwave");
  }

  if (allocated_) {
    for (int i = 0; i < options.nwave() * options.ncol(); ++i) {
      c_disort_state_free(&ds_[i]);
      c_disort_out_free(&ds_[i], &ds_out_[i]);
    }
  }

  ds_.resize(options.nwave() * options.ncol());
  ds_out_.resize(options.nwave() * options.ncol());

  for (int i = 0; i < options.nwave() * options.ncol(); ++i) {
    ds_[i] = options.ds();
    c_disort_state_alloc(&ds_[i]);
    c_disort_out_alloc(&ds_[i], &ds_out_[i]);

    if (ds_[i].flag.usrtau) {
      for (int j = 0; j < options.user_tau().size(); ++j)
        ds_[i].utau[j] = options.user_tau()[j];
    }

    if (ds_[i].flag.usrang) {
      for (int j = 0; j < options.user_mu().size(); ++j)
        ds_[i].umu[j] = options.user_mu()[j];

      for (int j = 0; j < options.user_phi().size(); ++j)
        ds_[i].phi[j] = options.user_phi()[j];
    }

    if (ds_[i].flag.planck) {
      ds_[i].wvnmlo = options.wave_lower()[i];
      ds_[i].wvnmhi = options.wave_upper()[i];
    } else {
      ds_[i].wvnmlo = 0.;
      ds_[i].wvnmhi = 1.;
    }
  }

  allocated_ = true;
}

DisortImpl::~DisortImpl() {
  for (int i = 0; i < options.nwave() * options.ncol(); ++i) {
    c_disort_state_free(&ds_[i]);
    c_disort_out_free(&ds_[i], &ds_out_[i]);
  }
  allocated_ = false;
}

torch::Tensor DisortImpl::get_flx(torch::TensorOptions op) const {
  int nlyr = options.ds().nlyr;
  auto result =
      torch::empty({options.nwave() * options.ncol(), nlyr + 1, 8}, op);

  for (int i = 0; i < options.nwave() * options.ncol(); ++i) {
    auto var =
        torch::from_blob(&ds_out_[i].rad[0].rfldir, {nlyr + 1, 8}, {8, 1}, op);
    result[i] = var;
  }

  return result.view({options.nwave(), options.ncol(), nlyr + 1, 8});
}

torch::Tensor DisortImpl::get_rad(torch::TensorOptions op) const {
  TORCH_CHECK(options.ds().flag.onlyfl == false,
              "DisortImpl::get_rad: ds.onlyfl == true");

  int nphi = options.ds().nphi;
  int ntau = options.ds().ntau;
  int numu = options.ds().numu;

  auto result =
      torch::empty({options.nwave() * options.ncol(), nphi, ntau, numu}, op);

  for (int i = 0; i < options.nwave() * options.ncol(); ++i) {
    auto var = torch::from_blob(ds_out_[i].uu, {nphi, ntau, numu},
                                {ntau * numu, numu, 1}, op);
    result[i] = var;
  }

  return result.view({options.nwave(), options.ncol(), nphi, ntau, numu});
}

//! \note Counting Disort Index
//! Example, il = 0, iu = 2, ds_.nlyr = 6, partition in to 3 blocks
//! face id   -> 0 - 1 - 2 - 3 - 4 - 5 - 6
//! cell id   -> | 0 | 1 | 2 | 3 | 4 | 5 |
//! disort id -> 6 - 5 - 4 - 3 - 2 - 1 - 0
//! blocks    -> ---------       *       *
//!           ->  r = 0  *       *       *
//!           ->         ---------       *
//!           ->           r = 1 *       *
//!           ->                 ---------
//!           ->                   r = 2
//! block r = 0 gets, 6 - 5 - 4
//! block r = 1 gets, 4 - 3 - 2
//! block r = 2 gets, 2 - 1 - 0
torch::Tensor DisortImpl::forward(torch::Tensor prop,
                                  std::map<std::string, torch::Tensor>* bc,
                                  torch::optional<torch::Tensor> temf) {
  TORCH_CHECK(options.ds().flag.ibcnd == 0,
              "DisortImpl::forward: ds.ibcnd != 0");

  // check dimensions
  TORCH_CHECK(prop.dim() == 4, "DisortImpl::forward: prop.dim() != 4");

  int nwave = prop.size(0);
  int ncol = prop.size(1);
  int nlyr = prop.size(2);

  // check ds
  TORCH_CHECK(options.ds().nlyr == nlyr,
              "DisortImpl::forward: ds.nlyr != nlyr");

  // check bc
  if (bc->find("fbeam") != bc->end()) {
    TORCH_CHECK(
        bc->at("fbeam").size(0) == nwave || bc->at("fbeam").size(0) == 1,
        "DisortImpl::forward: bc->fbeam.size(0) != nwave");
    TORCH_CHECK(bc->at("fbeam").size(1) == ncol || bc->at("fbeam").size(1) == 1,
                "DisortImpl::forward: bc->fbeam.size(1) != ncol");
  } else {
    (*bc)["fbeam"] = torch::zeros({nwave, ncol}, prop.options());
  }

  if (bc->find("umu0") != bc->end()) {
    TORCH_CHECK(bc->at("umu0").size(0) == nwave || bc->at("umu0").size(0) == 1,
                "DisortImpl::forward: bc->umu0.size(0) != nwave");
    TORCH_CHECK(bc->at("umu0").size(1) == ncol || bc->at("umu0").size(1) == 1,
                "DisortImpl::forward: bc->umu0.size(1) != ncol");
  } else {
    (*bc)["umu0"] = torch::ones({nwave, ncol}, prop.options());
  }

  if (bc->find("phi0") != bc->end()) {
    TORCH_CHECK(bc->at("phi0").size(0) == nwave || bc->at("phi0").size(0) == 1,
                "DisortImpl::forward: bc->phi0.size(0) != nwave");
    TORCH_CHECK(bc->at("phi0").size(1) == ncol || bc->at("phi0").size(1) == 1,
                "DisortImpl::forward: bc->phi0.size(1) != ncol");
  } else {
    (*bc)["phi0"] = torch::zeros({nwave, ncol}, prop.options());
  }

  if (bc->find("albedo") != bc->end()) {
    TORCH_CHECK(
        bc->at("albedo").size(0) == nwave || bc->at("albedo").size(0) == 1,
        "DisortImpl::forward: bc->albedo.size(0) != nwave");
    TORCH_CHECK(
        bc->at("albedo").size(1) == ncol || bc->at("albedo").size(1) == 1,
        "DisortImpl::forward: bc->albedo.size(1) != ncol");
  } else {
    (*bc)["albedo"] = torch::zeros({nwave, ncol}, prop.options());
  }

  if (bc->find("fluor") != bc->end()) {
    TORCH_CHECK(
        bc->at("fluor").size(0) == nwave || bc->at("fluor").size(0) == 1,
        "DisortImpl::forward: bc->fluor.size(0) != nwave");
    TORCH_CHECK(bc->at("fluor").size(1) == ncol || bc->at("fluor").size(1) == 1,
                "DisortImpl::forward: bc->fluor.size(1) != ncol");
  } else {
    (*bc)["fluor"] = torch::zeros({nwave, ncol}, prop.options());
  }

  if (bc->find("fisot") != bc->end()) {
    TORCH_CHECK(
        bc->at("fisot").size(0) == nwave || bc->at("fisot").size(0) == 1,
        "DisortImpl::forward: bc->fisot.size(0) != nwave");
    TORCH_CHECK(bc->at("fisot").size(1) == ncol || bc->at("fisot").size(1) == 1,
                "DisortImpl::forward: bc->fisot.size(1) != ncol");
  } else {
    (*bc)["fisot"] = torch::zeros({nwave, ncol}, prop.options());
  }

  torch::Tensor tem;
  if (temf.has_value()) {
    TORCH_CHECK(temf.value().size(0) == ncol,
                "DisortImpl::forward: temf.size(0) != ncol");
    TORCH_CHECK(temf.value().size(1) == nlyr + 1,
                "DisortImpl::forward: temf.size(1) != nlyr + 1");
    tem = temf.value();
  } else {
    TORCH_CHECK(options.ds().flag.planck == 0,
                "DisortImpl::forward: ds.planck != 0");
    // dummy
    tem = torch::empty({1, 1}, prop.options());
  }

  auto flx = torch::zeros({nwave, ncol, nlyr + 1, 2}, prop.options());
  auto index = torch::range(0, nwave * ncol - 1, 1)
                   .to(torch::kInt64)
                   .view({nwave, ncol, 1, 1});
  int rank_in_column = 0;

  auto iter =
      at::TensorIteratorConfig()
          .resize_outputs(false)
          .check_all_same_dtype(false)
          .declare_static_shape({nwave, ncol, nlyr + 1, 2},
                                /*squash_dims=*/{2, 3})
          .add_output(flx)
          .add_input(prop)
          .add_owned_const_input(bc->at("fbeam").unsqueeze(-1).unsqueeze(-1))
          .add_owned_const_input(bc->at("umu0").unsqueeze(-1).unsqueeze(-1))
          .add_owned_const_input(bc->at("phi0").unsqueeze(-1).unsqueeze(-1))
          .add_owned_const_input(bc->at("albedo").unsqueeze(-1).unsqueeze(-1))
          .add_owned_const_input(bc->at("fluor").unsqueeze(-1).unsqueeze(-1))
          .add_owned_const_input(bc->at("fisot").unsqueeze(-1).unsqueeze(-1))
          .add_owned_const_input(tem.unsqueeze(0).unsqueeze(-1))
          .add_input(index)
          .build();

  if (prop.is_cpu()) {
    call_disort_cpu(iter, rank_in_column, ds_.data(), ds_out_.data());
  } else if (prop.is_cuda()) {
#if defined(__CUDACC__)
    call_disort_cuda(iter, rank_in_column, ds_.data(), ds_out_.data());
#else
    TORCH_CHECK(false, "DisortImpl::forward: CUDA is not available");
#endif
  } else {
    TORCH_CHECK(false, "DisortImpl::forward: unsupported device");
  }

  return flx;
}

void print_ds_atm(std::ostream& os, disort_state const& ds) {
  os << "- Levels = " << ds.nlyr << std::endl;
  os << "- Radiation Streams = " << ds.nstr << std::endl;
  os << "- Phase function moments = " << ds.nmom << std::endl;
}

void print_ds_out(std::ostream& os, disort_state const& ds) {
  os << "- User azimuthal angles = " << ds.nphi << std::endl << "  : ";
  for (int i = 0; i < ds.nphi; ++i) {
    os << ds.phi[i] / M_PI * 180. << ", ";
  }
  os << std::endl;
  os << "- User polar angles = " << ds.numu << std::endl << "  : ";
  for (int i = 0; i < ds.numu; ++i) {
    os << acos(ds.umu[i]) / M_PI * 180. << ", ";
  }
  os << std::endl;
  os << "- User optical depths = " << ds.ntau << std::endl << "  : ";
  for (int i = 0; i < ds.ntau; ++i) {
    os << ds.utau[i] << ", ";
  }
  os << std::endl;
}

void DisortImpl::pretty_print(std::ostream& stream) const {
  std::cout << "Options: " << fmt::format("{}", options) << std::endl;
  std::cout << "Disort is configured with:" << std::endl;
  print_ds_flags(std::cout, options.ds());
}

void print_ds_bc(std::ostream& os, disort_state const& ds) {
  os << "- Bottom temperature = " << ds.bc.btemp << std::endl;
  os << "- Albedo = " << ds.bc.albedo << std::endl;
  os << "- Top temperature = " << ds.bc.ttemp << std::endl;
  os << "- Top emissivity = " << ds.bc.temis << std::endl;
  os << "- Bottom isotropic illumination = " << ds.bc.fluor << std::endl;
  os << "- Top isotropic illumination = " << ds.bc.fisot << std::endl;
  os << "- Solar beam = " << ds.bc.fbeam << std::endl;
  os << "- Cosine of solar zenith angle = " << ds.bc.umu0 << std::endl;
  os << "- Solar azimuth angle = " << ds.bc.phi0 << std::endl;
}

void print_ds_flags(std::ostream& os, disort_state const& ds) {
  if (ds.flag.ibcnd) {
    os << "- Spectral boundary condition (ibcnd) = True" << std::endl;
  } else {
    os << "- Spectral boundary condition (ibcnd) = False" << std::endl;
  }

  if (ds.flag.usrtau) {
    os << "- User optical depth (usrtau) = True" << std::endl;
  } else {
    os << "- User optical depth (usrtau) = False" << std::endl;
  }

  if (ds.flag.usrang) {
    os << "- User angles (usrang) = True" << std::endl;
  } else {
    os << "- User angles (usrang) = False" << std::endl;
  }

  if (ds.flag.lamber) {
    os << "- Lambertian surface (lamber) = True" << std::endl;
  } else {
    os << "- Lambertian surface (lamber) = False" << std::endl;
  }

  if (ds.flag.planck) {
    os << "- Planck function (planck) = True" << std::endl;
  } else {
    os << "- Planck function (planck) = False" << std::endl;
  }

  if (ds.flag.spher) {
    os << "- Spherical correction (spher) = True" << std::endl;
  } else {
    os << "- Spherical correction (spher) = False" << std::endl;
  }

  if (ds.flag.onlyfl) {
    os << "- Only calculate fluxes (onlyfl) = True" << std::endl;
  } else {
    os << "- Only calculate fluxes (onlyfl) = False" << std::endl;
  }

  if (ds.flag.intensity_correction) {
    os << "- Intensity correction (intensity_correction) = True" << std::endl;
  } else {
    os << "- Intensity correction (intensity_correction) = False" << std::endl;
  }

  if (ds.flag.old_intensity_correction) {
    os << "- Old intensity correction (old_intensity_correction) = True"
       << std::endl;
  } else {
    os << "- Old intensity correction (old_intensity_correction) = False"
       << std::endl;
  }

  if (ds.flag.general_source) {
    os << "- General source function (general_source) = True" << std::endl;
  } else {
    os << "- General source function (general_source) = False" << std::endl;
  }

  if (ds.flag.output_uum) {
    os << "- Output uum (output_uum) = True" << std::endl;
  } else {
    os << "- Output uum (output_uum) = False" << std::endl;
  }
}

}  // namespace disort
