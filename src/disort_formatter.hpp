#pragma once

// fmt
#include <fmt/format.h>

// fvm
#include "disort.hpp"

template <>
struct fmt::formatter<disort::DisortOptions> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const disort::DisortOptions& p, FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "(flags = {}; nwave = {}, ncol = {})",
                          p.flags(), p.nwave(), p.ncol());
  }
};
