#pragma once

// C/C++
#include <string>

// fmt
#include <fmt/format.h>

// disort
#include "disort.hpp"
#include "scattering_moments.hpp"

template <>
struct fmt::formatter<disort_state> {
  constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const disort_state &ds, FormatContext &ctx) const {
    return fmt::format_to(
        ctx.out(),
        "(nlyr = {}; nstr = {}; nmom = {}; ibcnd = {}; usrtau = {}; usrang = "
        "{}; lamber = {}; planck = {}; spher = {}; onlyfl = {})",
        ds.nlyr, ds.nstr, ds.nmom, ds.flag.ibcnd, ds.flag.usrtau,
        ds.flag.usrang, ds.flag.lamber, ds.flag.planck, ds.flag.spher,
        ds.flag.onlyfl);
  }
};

template <>
struct fmt::formatter<disort::DisortOptions> {
  constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const disort::DisortOptions &p, FormatContext &ctx) const {
    int nwave = std::min(p.nwave(), static_cast<int>(p.wave_lower().size()));
    nwave = std::min(nwave, static_cast<int>(p.wave_upper().size()));

    std::string waves = "(";
    if (p.flags().find("planck") != std::string::npos) {
      for (int i = 0; i < nwave; ++i) {
        waves += fmt::format("({},{})", p.wave_lower()[i], p.wave_upper()[i]);
        if (i < nwave - 1) {
          waves += ", ";
        }
      }
    }
    waves += ")";

    return fmt::format_to(
        ctx.out(),
        "(flags = {}; nwave = {}; ncol = {}; wave = {}; disort_state = {})",
        p.flags(), p.nwave(), p.ncol(), waves, p.ds());
  }
};
