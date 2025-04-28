// fmt
#include <fmt/format.h>

// disort
#include <disort/disort.hpp>
#include <disort/disort_formatter.hpp>
#include <disort/scattering_moments.hpp>

int main(int argc, char **argv) {
  disort::DisortOptions op;

  op.header("running disort example");
  op.flags(
      "usrtau,usrang,lamber,quiet,intensity_correction,"
      "old_intensity_correction,print-input,print-phase-function");

  op.nwave(10);
  op.ds().nlyr = 1;
  op.ds().nstr = 16;
  op.ds().nmom = 16;
  op.ds().nphase = 16;

  op.user_mu({-1, -0.5, -0.1, 0.1, 0.5, 1});
  op.user_phi({0});
  op.user_tau({0, 0.03125});

  disort::Disort disort(op);

  auto prop = torch::zeros({disort->options.nwave(), disort->options.ncol(),
                            disort->ds().nlyr, 2 + disort->ds().nstr},
                           torch::kDouble);

  prop.select(3, disort::index::IEX) = disort->ds().utau[1];
  prop.select(3, disort::index::ISS) = 0.2;
  prop.narrow(3, disort::index::IPM, disort->ds().nstr) =
      disort::scattering_moments(disort->ds().nstr);

  std::map<std::string, torch::Tensor> bc;

  bc["umu0"] = 0.1 * torch::ones({disort->options.ncol()}, torch::kDouble);
  bc["fbeam"] = (M_PI / bc["umu0"])
                    .unsqueeze(0)
                    .expand({disort->options.nwave(), disort->options.ncol()});

  auto result = disort->forward(prop, &bc);
  std::cout << "result: " << result << std::endl;

  auto rad = disort->gather_rad();
  std::cout << "rad = " << rad << std::endl;

  std::cout << "options = " << fmt::format("{}", disort->options) << std::endl;
}
