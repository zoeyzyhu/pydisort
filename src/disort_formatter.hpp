#pragma once

// C/C++
#include <string>

// fmt
#include <fmt/format.h>

// fvm
#include "disort.hpp"
#include "scattering_moments.hpp"

template <>
struct fmt::formatter<disort_state> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const disort_state& ds, FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "(nlyr = {}; nstr = {}; nmom = {})",
                          ds.nlyr, ds.nstr, ds.nmom);
  }
};

template <>
struct fmt::formatter<disort::DisortOptions> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const disort::DisortOptions& p, FormatContext& ctx) const {
    std::string waves = "(";
    if (p.ds().flag.planck) {
      for (int i = 0; i < p.nwave(); ++i) {
        waves += fmt::format("({},{})", p.wave_lower()[i], p.wave_upper()[i]);
        if (i < p.nwave() - 1) {
          waves += ", ";
        }
      }
    }
    waves += ")";

    return fmt::format_to(
        ctx.out(),
        "(flags = {}; nwave = {}; ncol = {}; disort_state = {}; wave = {})",
        p.flags(), p.nwave(), p.ncol(), p.ds(), waves);
  }
};

template <>
struct fmt::formatter<disort::PhaseMomentOptions> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const disort::PhaseMomentOptions& p, FormatContext& ctx) const {
    std::string type_str;

    switch (p.type()) {
      case disort::kIsotropic:
        type_str = "isotropic";
        break;
      case disort::kRayleigh:
        type_str = "rayleigh";
        break;
      case disort::kHenyeyGreenstein:
        type_str = "henyey-greenstein";
        break;
      case disort::kDoubleHenyeyGreenstein:
        type_str = "double-henyey-greenstein";
        break;
      case disort::kHazeGarciaSiewert:
        type_str = "haze-garcia-siewert";
        break;
      case disort::kCloudGarciaSiewert:
        type_str = "cloud-garcia-siewert";
        break;
      default:
        type_str = "unknown";
        break;
    }
    return fmt::format_to(ctx.out(),
                          "(type = {}; gg = {}; gg1 = {}; gg2 = {}; ff = {})",
                          type_str, p.gg(), p.gg1(), p.gg2(), p.ff());
  }
};
