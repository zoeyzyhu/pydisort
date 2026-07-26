// C/C++
#include <map>
#include <vector>

// disort
#include "disort.hpp"
#include "disort_dispatch.hpp"
#include "disort_formatter.hpp"
#include "vectorize.hpp"

namespace disort {

DisortOptionsImpl::DisortOptionsImpl() {
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
  ds().flag.brdf_type = 0;

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

void DisortOptionsImpl::set_header(std::string const &header) {
  snprintf(ds().header, sizeof(ds().header), "%s", header.c_str());
}

void DisortOptionsImpl::set_flags(std::string const &str) {
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
      throw std::runtime_error("DisortOptionsImpl::set_flags::" + msg.str());
    }
  }
}

DisortImpl::DisortImpl(DisortOptions const &options_) : options(options_) {
  reset();
}

// Note: reset() modifies the shared options object. If multiple DisortImpl
// instances share the same DisortOptions, all instances will see these changes.
void DisortImpl::reset() {
  options->set_header(options->header());
  options->set_flags(options->flags());

  options->ds().accur = options->accur();

  options->ds().nphi = options->user_phi().size();
  options->ds().numu = options->user_mu().size();
  options->ds().ntau = options->user_tau().size();

  TORCH_CHECK(options->ds().nlyr > 0, "DisortImpl: ds.nlyr <= 0");
  TORCH_CHECK(options->ds().nstr > 0, "DisortImpl: ds.nstr <= 0");
  TORCH_CHECK(options->ds().nmom >= options->ds().nstr,
              "DisortImpl: ds.nmom < ds.nstr");

  if (options->ds().flag.planck) {
    TORCH_CHECK(options->wave_lower().size() == options->nwave(),
                "DisortImpl: wave_lower.size() != nwave");
    TORCH_CHECK(options->wave_upper().size() == options->nwave(),
                "DisortImpl: wave_upper.size() != nwave");
  }

  if (allocated_) {
    for (int i = 0; i < options->nwave() * options->ncol(); ++i) {
      c_disort_state_free(&ds_[i]);
      c_disort_out_free(&ds_[i], &ds_out_[i]);
    }
  }

  ds_.resize(options->nwave() * options->ncol());
  ds_out_.resize(options->nwave() * options->ncol());

  for (int i = 0; i < options->nwave() * options->ncol(); ++i) {
    ds_[i] = options->ds();
    c_disort_state_alloc(&ds_[i]);
    c_disort_out_alloc(&ds_[i], &ds_out_[i]);

    if (ds_[i].flag.usrtau) {
      for (int j = 0; j < options->user_tau().size(); ++j)
        ds_[i].utau[j] = options->user_tau()[j];
    }

    if (ds_[i].flag.usrang) {
      for (int j = 0; j < options->user_mu().size(); ++j)
        ds_[i].umu[j] = options->user_mu()[j];

      for (int j = 0; j < options->user_phi().size(); ++j)
        ds_[i].phi[j] = options->user_phi()[j];
    }

    if (ds_[i].flag.planck) {
      ds_[i].wvnmlo = options->wave_lower()[i / options->ncol()];
      ds_[i].wvnmhi = options->wave_upper()[i / options->ncol()];
    } else {
      ds_[i].wvnmlo = 0.;
      ds_[i].wvnmhi = 1.;
    }
  }

  allocated_ = true;
}

DisortImpl::~DisortImpl() {
  if (allocated_) {
    for (int i = 0; i < options->nwave() * options->ncol(); ++i) {
      c_disort_state_free(&ds_[i]);
      c_disort_out_free(&ds_[i], &ds_out_[i]);
    }
  }
  allocated_ = false;
}

torch::Tensor DisortImpl::gather_flx() const {
  TORCH_CHECK(allocated_, "DisortImpl::gather_flx: DisortImpl not allocated");

  int nlyr = options->ds().nlyr;
  auto result = torch::empty({options->nwave() * options->ncol(), nlyr + 1, 8},
                             result_options_);

  for (int i = 0; i < options->nwave() * options->ncol(); ++i) {
    auto var = torch::from_blob(&ds_out_[i].rad[0].rfldir, {nlyr + 1, 8},
                                {8, 1}, result_options_.dtype(torch::kFloat64));
    result[i].copy_(var);
  }

  if (options->upward()) {
    return result.view({options->nwave(), options->ncol(), nlyr + 1, 8})
        .flip(2);
  } else {
    return result.view({options->nwave(), options->ncol(), nlyr + 1, 8});
  }
}

torch::Tensor DisortImpl::gather_rad() const {
  TORCH_CHECK(allocated_, "DisortImpl::gather_rad: DisortImpl not allocated");

  TORCH_CHECK(options->ds().flag.onlyfl == false,
              "DisortImpl::gather_rad: ds.onlyfl == true");

  int nphi = options->ds().nphi;
  int ntau = options->ds().ntau;
  int numu = options->ds().numu;

  auto result = torch::empty(
      {options->nwave() * options->ncol(), nphi, ntau, numu}, result_options_);

  for (int i = 0; i < options->nwave() * options->ncol(); ++i) {
    auto var = torch::from_blob(ds_out_[i].uu, {nphi, ntau, numu},
                                {ntau * numu, numu, 1},
                                result_options_.dtype(torch::kFloat64));
    result[i].copy_(var);
  }

  return result.view({options->nwave(), options->ncol(), nphi, ntau, numu});
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
                                  std::map<std::string, torch::Tensor> *bc,
                                  std::string bname,
                                  torch::optional<torch::Tensor> temf) {
  TORCH_CHECK(options->ds().flag.ibcnd == 0,
              "DisortImpl::forward: ds.ibcnd != 0");

  // check dimensions
  TORCH_CHECK(prop.dim() == 4, "DisortImpl::forward: prop.dim() != 4");

  int nwave = prop.size(0);
  int ncol = prop.size(1);
  int nlyr = prop.size(2);

  TORCH_CHECK(options->nwave() == nwave,
              "DisortImpl::forward: options->nwave != prop.size(0)");

  TORCH_CHECK(options->ncol() == ncol,
              "DisortImpl::forward: options->ncol != prop.size(1)");

  // check ds
  TORCH_CHECK(options->ds().nlyr == nlyr,
              "DisortImpl::forward: ds.nlyr != nlyr");

  // add slash
  if (bname.size() > 0 && bname.back() != '/') {
    bname += "/";
  }

  // check bc
  if (bc->find(bname + "umu0") != bc->end()) {
    TORCH_CHECK(bc->at(bname + "umu0").dim() == 1,
                "DisortImpl::forward: bc->umu0.dim() != 1");
    TORCH_CHECK(bc->at(bname + "umu0").size(0) == ncol,
                "DisortImpl::forward: bc->umu0.size(0) != ncol");
    (*bc)["umu0"] = bc->at(bname + "umu0");
  } else {
    (*bc)["umu0"] = torch::ones({1, ncol}, prop.options());
  }

  if (bc->find(bname + "phi0") != bc->end()) {
    TORCH_CHECK(bc->at(bname + "phi0").dim() == 1,
                "DisortImpl::forward: bc->phi0.dim() != 1");
    TORCH_CHECK(bc->at(bname + "phi0").size(0) == ncol,
                "DisortImpl::forward: bc->phi0.size(0) != ncol");
    (*bc)["phi0"] = bc->at(bname + "phi0");
  } else {
    (*bc)["phi0"] = torch::zeros({1, ncol}, prop.options());
  }

  if (bc->find(bname + "fbeam") != bc->end()) {
    TORCH_CHECK(bc->at(bname + "fbeam").dim() == 2,
                "DisortImpl::forward: bc->fbeam.dim() != 2");
    TORCH_CHECK(bc->at(bname + "fbeam").size(0) == nwave,
                "DisortImpl::forward: bc->fbeam.size(0) != nwave");
    TORCH_CHECK(bc->at(bname + "fbeam").size(1) == ncol,
                "DisortImpl::forward: bc->fbeam.size(1) != ncol");
    (*bc)["fbeam"] = bc->at(bname + "fbeam");
  } else {
    (*bc)["fbeam"] = torch::zeros({nwave, ncol}, prop.options());
  }

  if (bc->find(bname + "albedo") != bc->end()) {
    TORCH_CHECK(bc->at(bname + "albedo").dim() == 2,
                "DisortImpl::forward: bc->albedo.dim() != 2");
    TORCH_CHECK(bc->at(bname + "albedo").size(0) == nwave,
                "DisortImpl::forward: bc->albedo.size(0) != nwave");
    TORCH_CHECK(bc->at(bname + "albedo").size(1) == ncol,
                "DisortImpl::forward: bc->albedo.size(1) != ncol");
    (*bc)["albedo"] = bc->at(bname + "albedo");
  } else {
    (*bc)["albedo"] = torch::zeros({nwave, ncol}, prop.options());
  }

  if (bc->find(bname + "fluor") != bc->end()) {
    TORCH_CHECK(bc->at(bname + "fluor").dim() == 2,
                "DisortImpl::forward: bc->fluor.dim() != 2");
    TORCH_CHECK(bc->at(bname + "fluor").size(0) == nwave,
                "DisortImpl::forward: bc->fluor.size(0) != nwave");
    TORCH_CHECK(bc->at(bname + "fluor").size(1) == ncol,
                "DisortImpl::forward: bc->fluor.size(1) != ncol");
    (*bc)["fluor"] = bc->at(bname + "fluor");
  } else {
    (*bc)["fluor"] = torch::zeros({nwave, ncol}, prop.options());
  }

  if (bc->find(bname + "fisot") != bc->end()) {
    TORCH_CHECK(bc->at(bname + "fisot").dim() == 2,
                "DisortImpl::forward: bc->fisot.dim() != 2");
    TORCH_CHECK(bc->at(bname + "fisot").size(0) == nwave,
                "DisortImpl::forward: bc->fisot.size(0) != nwave");
    TORCH_CHECK(bc->at(bname + "fisot").size(1) == ncol,
                "DisortImpl::forward: bc->fisot.size(1) != ncol");
    (*bc)["fisot"] = bc->at(bname + "fisot");
  } else {
    (*bc)["fisot"] = torch::zeros({nwave, ncol}, prop.options());
  }

  if (bc->find(bname + "temis") != bc->end()) {
    TORCH_CHECK(bc->at(bname + "temis").dim() == 2,
                "DisortImpl::forward: bc->temis.dim() != 2");
    TORCH_CHECK(bc->at(bname + "temis").size(0) == nwave,
                "DisortImpl::forward: bc->temis.size(0) != nwave");
    TORCH_CHECK(bc->at(bname + "temis").size(1) == ncol,
                "DisortImpl::forward: bc->temis.size(1) != ncol");
    (*bc)["temis"] = bc->at(bname + "temis");
  } else {
    (*bc)["temis"] = torch::zeros({nwave, ncol}, prop.options());
  }

  if (bc->find("btemp") != bc->end()) {
    TORCH_CHECK(bc->at("btemp").dim() == 1,
                "DisortImpl::forward: bc->btemp.dim() != 1");
    TORCH_CHECK(bc->at("btemp").size(0) == ncol,
                "DisortImpl::forward: bc->btemp.size(0) != ncol");
  } else {
    (*bc)["btemp"] = torch::zeros({1, ncol}, prop.options());
  }

  if (bc->find("ttemp") != bc->end()) {
    TORCH_CHECK(bc->at("ttemp").dim() == 1,
                "DisortImpl::forward: bc->ttemp.dim() != 1");
    TORCH_CHECK(bc->at("ttemp").size(0) == ncol,
                "DisortImpl::forward: bc->ttemp.size(0) != ncol");
  } else {
    (*bc)["ttemp"] = torch::zeros({1, ncol}, prop.options());
  }

  torch::Tensor tem;
  if (temf.has_value()) {
    TORCH_CHECK(temf.value().dim() == 2,
                "DisortImpl::forward: temf.dim() != 2");
    TORCH_CHECK(temf.value().size(0) == ncol,
                "DisortImpl::forward: temf.size(0) != ncol");
    TORCH_CHECK(temf.value().size(1) == nlyr + 1,
                "DisortImpl::forward: temf.size(1) != nlyr + 1");
    tem = temf.value();
  } else {
    TORCH_CHECK(options->ds().flag.planck == 0,
                "DisortImpl::forward: ds.planck != 0");
    // dummy
    tem = torch::empty({ncol, nlyr + 1}, prop.options());
  }

  auto flx = prop.is_cuda()
                 ? torch::empty({nwave, ncol, ds().ntau, 2}, prop.options())
                 : torch::zeros({nwave, ncol, ds().ntau, 2}, prop.options());
  auto build_iterator = [&](torch::Tensor& output, const torch::Tensor& prop_in,
                            const torch::Tensor& umu0, const torch::Tensor& phi0,
                            const torch::Tensor& fbeam,
                            const torch::Tensor& albedo,
                            const torch::Tensor& fluor,
                            const torch::Tensor& fisot,
                            const torch::Tensor& temis,
                            const torch::Tensor& btemp,
                            const torch::Tensor& ttemp,
                            const torch::Tensor& tem_in) {
    const int64_t input_nwave = prop_in.size(0);
    const int64_t input_ncol = prop_in.size(1);
    at::TensorIteratorConfig iter_config;
    iter_config.resize_outputs(false)
        .check_all_same_dtype(true)
        .declare_static_shape({input_nwave, input_ncol, ds().ntau, 2},
                              /*squash_dims=*/{2, 3})
        .add_output(output)
        .add_input(prop_in)
        .add_owned_input(
            umu0.view({1, input_ncol, 1, 1})
                .expand({input_nwave, input_ncol, 1, 1}))
        .add_owned_input(
            phi0.view({1, input_ncol, 1, 1})
                .expand({input_nwave, input_ncol, 1, 1}))
        .add_owned_input(fbeam.view({input_nwave, input_ncol, 1, 1}))
        .add_owned_input(albedo.view({input_nwave, input_ncol, 1, 1}))
        .add_owned_input(fluor.view({input_nwave, input_ncol, 1, 1}))
        .add_owned_input(fisot.view({input_nwave, input_ncol, 1, 1}))
        .add_owned_input(temis.view({input_nwave, input_ncol, 1, 1}))
        .add_owned_input(
            btemp.view({1, input_ncol, 1, 1})
                .expand({input_nwave, input_ncol, 1, 1}))
        .add_owned_input(
            ttemp.view({1, input_ncol, 1, 1})
                .expand({input_nwave, input_ncol, 1, 1}))
        .add_owned_input(tem_in.view({1, input_ncol, nlyr + 1, 1})
                             .expand({input_nwave, input_ncol, nlyr + 1, 1}));
    if (!output.is_cuda()) {
      auto index = torch::arange(input_nwave * input_ncol, prop_in.options())
                       .view({input_nwave, input_ncol, 1, 1});
      iter_config.add_owned_input(index);
    }
    return iter_config.build();
  };

  constexpr double kConservativeScatteringThreshold = 1.e-8;
  const bool fast_flux_eligible =
      prop.is_cuda() && c_fast_flux_eligible(&ds());
  const bool route_conservative = fast_flux_eligible && prop.size(3) > 1;
  auto run_disort = [&](at::TensorIterator& iter, bool general_path,
                        disort_state* states) {
    at::native::call_disort(flx.device().type(), iter, options->upward(),
                            general_path, states, ds_out_.data(),
                            &cuda_workspace_);
  };

  bool general_path = false;
  if (route_conservative) {
    const auto conservative_mask =
        prop.select(-1, 1).gt(1. - kConservativeScatteringThreshold).any(-1);
    const int64_t nconservative = conservative_mask.sum().item<int64_t>();
    const int64_t nwork = nwave * ncol;

    if (nconservative > 0 && nconservative < nwork) {
      const auto conservative_indices =
          torch::nonzero(conservative_mask.reshape({-1})).reshape({-1});
      auto fast_iter = build_iterator(
          flx, prop, bc->at("umu0"), bc->at("phi0"), bc->at("fbeam"),
          bc->at("albedo"), bc->at("fluor"), bc->at("fisot"),
          bc->at("temis"), bc->at("btemp"), bc->at("ttemp"), tem);
      run_disort(fast_iter, false, ds_.data());

      const auto host_indices = conservative_indices.cpu().contiguous();
      const auto index_ptr = host_indices.data_ptr<int64_t>();
      std::vector<disort_state> general_ds;
      general_ds.reserve(nconservative);
      for (int64_t i = 0; i < nconservative; ++i) {
        general_ds.push_back(ds_[index_ptr[i]]);
      }

      const auto column_indices = conservative_indices.remainder(ncol);
      auto select_column = [&](const torch::Tensor& input) {
        return input.reshape({ncol}).index_select(0, column_indices);
      };
      auto select_wave_column = [&](const torch::Tensor& input) {
        return input.reshape({nwave * ncol})
            .index_select(0, conservative_indices)
            .view({1, nconservative});
      };
      auto sub_prop = prop.flatten(0, 1)
                          .index_select(0, conservative_indices)
                          .unsqueeze(0);
      auto sub_flx = torch::empty({1, nconservative, ds().ntau, 2},
                                  prop.options());
      auto general_iter = build_iterator(
          sub_flx, sub_prop, select_column(bc->at("umu0")),
          select_column(bc->at("phi0")), select_wave_column(bc->at("fbeam")),
          select_wave_column(bc->at("albedo")),
          select_wave_column(bc->at("fluor")),
          select_wave_column(bc->at("fisot")),
          select_wave_column(bc->at("temis")), select_column(bc->at("btemp")),
          select_column(bc->at("ttemp")), tem.index_select(0, column_indices));
      run_disort(general_iter, true, general_ds.data());
      flx.view({nwave * ncol, ds().ntau, 2})
          .index_copy_(0, conservative_indices,
                       sub_flx.view({nconservative, ds().ntau, 2}));
      result_options_ = flx.options();
      return flx;
    }
    general_path = nconservative == nwork;
  }

  auto iter = build_iterator(
      flx, prop, bc->at("umu0"), bc->at("phi0"), bc->at("fbeam"),
      bc->at("albedo"), bc->at("fluor"), bc->at("fisot"), bc->at("temis"),
      bc->at("btemp"), bc->at("ttemp"), tem);
  run_disort(iter, general_path, ds_.data());

  // save result tensor options
  result_options_ = flx.options();

  return flx;
}

void print_ds_atm(std::ostream &os, disort_state const &ds) {
  os << "- Levels = " << ds.nlyr << std::endl;
  os << "- Radiation Streams = " << ds.nstr << std::endl;
  os << "- Phase function moments = " << ds.nmom << std::endl;
}

void print_ds_out(std::ostream &os, disort_state const &ds) {
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

void DisortImpl::pretty_print(std::ostream &stream) const {
  std::cout << "Options: " << fmt::format("{}", options) << std::endl;
  std::cout << "Disort is configured with:" << std::endl;
  print_ds_flags(std::cout, options->ds());
}

void print_ds_bc(std::ostream &os, disort_state const &ds) {
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

void print_ds_flags(std::ostream &os, disort_state const &ds) {
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
