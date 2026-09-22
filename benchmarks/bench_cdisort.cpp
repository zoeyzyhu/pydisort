/******************************************************************************
 * bench_cdisort.cpp
 *
 * Standalone cdisort driver for DISORT Test Problem 9, used as the C baseline
 * in `compare_cdisort.py`. It is a benchmarking counterpart to
 * `tests/cdisort213/test_cdisort_09.c`, with three differences that matter for
 * a fair comparison against pydisort:
 *
 *   1. It times the solve loop *internally* (std::chrono), so process startup,
 *      dynamic loading and argument parsing are not charged to cdisort. At
 *      nwave = 20 that startup is roughly half the wall time of the test
 *      driver, which is enough to misreport the per-solve cost by 2x.
 *
 *   2. It can hoist the state allocation out of the loop (--alloc hoisted).
 *      `test_cdisort_09.c` calls c_disort_state_alloc / c_disort_out_alloc /
 *      *_free on every iteration, whereas pydisort allocates its disort_state
 *      array once in reset() and reuses it across the whole batch. Timing the
 *      two against each other without this switch charges cdisort for
 *      allocations its competitor never performs. The per-layer inputs
 *      (dtauc, ssalb, pmom) are re-written on every iteration in both modes,
 *      because pydisort re-copies them out of the `prop` tensor for every
 *      (wave, column) element -- so "hoisted" is exactly pydisort's work, and
 *      "percall" is exactly the test driver's.
 *
 *   3. It can print the fluxes (--verify), so the Python side can confirm the
 *      two implementations agree before any timing is reported.
 *
 * The physical problem is transcribed from `tests/cdisort213/test_cdisort_09.c`
 * and is identical to it at matching arguments.
 *
 * Build (the Python driver does this for you):
 *   c++ -std=c++17 -O3 -funroll-loops -fstrict-aliasing -DNDEBUG \
 *       -I<repo root> bench_cdisort.cpp -o bench_cdisort -lm
 *
 * Usage:
 *   bench_cdisort [--nstr N] [--nlyr N] [--nwave N] [--ssalb F]
 *                 [--mode radiance|flux] [--alloc percall|hoisted]
 *                 [--repeat N] [--verify]
 *****************************************************************************/

#include <cdisort213/cdisort.hpp>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

#undef DTAUC
#define DTAUC(lc) ds.dtauc[(lc) - 1]
#undef PHI
#define PHI(j) ds.phi[(j) - 1]
#undef PMOM
#define PMOM(k, lc) ds.pmom[(k) + ((lc) - 1) * (ds.nmom_nstr + 1)]
#undef SSALB
#define SSALB(lc) ds.ssalb[(lc) - 1]
#undef UMU
#define UMU(iu) ds.umu[(iu) - 1]
#undef UTAU
#define UTAU(lu) ds.utau[(lu) - 1]

namespace {

// Output grid, boundary conditions and geometry of Test Problem 9, case 1.
// Kept in one place so the Python side can assert it matches its own copy.
const double kUserTau[5] = {0.0, 1.05, 2.1, 6.0, 21.0};
const double kUserMu[4] = {-1.0, -0.2, 0.2, 1.0};
const double kUserPhi[1] = {60.0};

const double kUmu0 = 0.5;
const double kPhi0 = 0.0;
const double kFbeam = 0.0;
const double kFluor = 0.0;
const double kAlbedo = 0.0;
// fisot = 1/pi, so the downward flux at the top of the atmosphere is exactly 1.
const double kFisot = 1.0 / M_PI;

struct Config {
  int nstr = 32;
  int nlyr = 100;
  int nwave = 1000;
  int repeat = 1;
  double ssalb = 0.003;
  bool radiance = true;
  bool hoist_alloc = false;
  bool verify = false;
};

//! Configure flags and dimensions; must run before c_disort_state_alloc.
void configure(disort_state& ds, const Config& cfg) {
  ds.accur = 0.;
  for (int i = 0; i < 5; ++i) ds.flag.prnt[i] = FALSE;

  ds.flag.ibcnd = GENERAL_BC;
  ds.flag.usrtau = TRUE;
  ds.flag.usrang = cfg.radiance ? TRUE : FALSE;
  ds.flag.lamber = TRUE;
  ds.flag.onlyfl = cfg.radiance ? FALSE : TRUE;
  ds.flag.quiet = TRUE;
  ds.flag.spher = FALSE;
  ds.flag.general_source = FALSE;
  ds.flag.output_uum = FALSE;
  ds.flag.intensity_correction = TRUE;
  ds.flag.old_intensity_correction = TRUE;
  ds.flag.planck = FALSE;
  ds.flag.brdf_type = BRDF_NONE;

  ds.nstr = cfg.nstr;
  ds.nlyr = cfg.nlyr;
  ds.nphase = ds.nstr;
  ds.nmom = ds.nstr;
  ds.ntau = 5;
  ds.numu = 4;
  ds.nphi = 1;

  ds.bc.fbeam = kFbeam;
  ds.bc.fisot = kFisot;
  ds.bc.phi0 = kPhi0;
  ds.bc.umu0 = kUmu0;
  ds.bc.fluor = kFluor;
  ds.bc.albedo = kAlbedo;
}

//! Output grid; constant across iterations, so it is set once after alloc.
void set_grids(disort_state& ds) {
  for (int i = 0; i < 5; ++i) UTAU(i + 1) = kUserTau[i];
  for (int i = 0; i < 4; ++i) UMU(i + 1) = kUserMu[i];
  PHI(1) = kUserPhi[0];
}

//! Per-layer optical properties.
/*!
 * Re-written on every iteration in both allocation modes, mirroring pydisort,
 * which re-copies dtauc/ssalb/pmom out of its `prop` tensor for every
 * (wave, column) element it solves.
 */
void set_medium(disort_state& ds, double ssalb_slope) {
  for (int lc = 1; lc <= ds.nlyr; ++lc) {
    DTAUC(lc) = (static_cast<double>(lc) / ds.nlyr) * 6.;
    SSALB(lc) = 0.6 + static_cast<double>(lc) * ssalb_slope;
    c_getmom(ISOTROPIC, 0., ds.nmom, &PMOM(0, lc));
  }
}

//! Fluxes at each user_tau, in pydisort's convention.
/*!
 * pydisort's forward() returns [upward, downward] with the downward component
 * being rfldir + rfldn (direct plus diffuse) -- see FLX(...) in
 * src/disort_impl.h. Matching that here is what makes the cross-check
 * meaningful.
 */
void print_fluxes(const disort_state& ds, const disort_output& out) {
  printf("# tau flux_up flux_down\n");
  for (int i = 0; i < ds.ntau; ++i) {
    printf("%.17g %.17g %.17g\n", ds.utau[i], out.rad[i].flup,
           out.rad[i].rfldir + out.rad[i].rfldn);
  }
}

//! One solve, allocating and freeing the state around it (test-driver style).
void solve_percall(const Config& cfg, disort_output* keep) {
  disort_state ds;
  disort_output out;

  configure(ds, cfg);
  c_disort_state_alloc(&ds);
  c_disort_out_alloc(&ds, &out);

  set_grids(ds);
  set_medium(ds, cfg.ssalb);

  c_disort(&ds, &out, c_planck_func2);

  if (keep) print_fluxes(ds, out);

  c_disort_out_free(&ds, &out);
  c_disort_state_free(&ds);
}

double now_seconds() {
  using clock = std::chrono::steady_clock;
  return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

//! Time `nwave` solves with allocation inside the loop.
double time_percall(const Config& cfg) {
  double best = std::numeric_limits<double>::infinity();
  for (int r = 0; r < cfg.repeat; ++r) {
    const double t0 = now_seconds();
    for (int i = 0; i < cfg.nwave; ++i) solve_percall(cfg, nullptr);
    const double dt = now_seconds() - t0;
    if (dt < best) best = dt;
  }
  return best;
}

//! Time `nwave` solves reusing one allocation, as pydisort does.
double time_hoisted(const Config& cfg) {
  disort_state ds;
  disort_output out;

  configure(ds, cfg);
  c_disort_state_alloc(&ds);
  c_disort_out_alloc(&ds, &out);
  set_grids(ds);

  double best = std::numeric_limits<double>::infinity();
  for (int r = 0; r < cfg.repeat; ++r) {
    const double t0 = now_seconds();
    for (int i = 0; i < cfg.nwave; ++i) {
      set_medium(ds, cfg.ssalb);
      c_disort(&ds, &out, c_planck_func2);
    }
    const double dt = now_seconds() - t0;
    if (dt < best) best = dt;
  }

  c_disort_out_free(&ds, &out);
  c_disort_state_free(&ds);
  return best;
}

int usage(const char* argv0) {
  fprintf(stderr,
          "Usage: %s [--nstr N] [--nlyr N] [--nwave N] [--ssalb F]\n"
          "          [--mode radiance|flux] [--alloc percall|hoisted]\n"
          "          [--repeat N] [--verify]\n",
          argv0);
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  Config cfg;

  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];
    const bool has_value = (i + 1 < argc);

    if (!strcmp(a, "--verify")) {
      cfg.verify = true;
    } else if (!strcmp(a, "--help") || !strcmp(a, "-h")) {
      return usage(argv[0]);
    } else if (!has_value) {
      fprintf(stderr, "error: %s needs a value\n", a);
      return usage(argv[0]);
    } else if (!strcmp(a, "--nstr")) {
      cfg.nstr = atoi(argv[++i]);
    } else if (!strcmp(a, "--nlyr")) {
      cfg.nlyr = atoi(argv[++i]);
    } else if (!strcmp(a, "--nwave")) {
      cfg.nwave = atoi(argv[++i]);
    } else if (!strcmp(a, "--repeat")) {
      cfg.repeat = atoi(argv[++i]);
    } else if (!strcmp(a, "--ssalb")) {
      cfg.ssalb = atof(argv[++i]);
    } else if (!strcmp(a, "--mode")) {
      const char* m = argv[++i];
      if (!strcmp(m, "radiance")) {
        cfg.radiance = true;
      } else if (!strcmp(m, "flux")) {
        cfg.radiance = false;
      } else {
        fprintf(stderr, "error: --mode must be radiance or flux\n");
        return usage(argv[0]);
      }
    } else if (!strcmp(a, "--alloc")) {
      const char* m = argv[++i];
      if (!strcmp(m, "percall")) {
        cfg.hoist_alloc = false;
      } else if (!strcmp(m, "hoisted")) {
        cfg.hoist_alloc = true;
      } else {
        fprintf(stderr, "error: --alloc must be percall or hoisted\n");
        return usage(argv[0]);
      }
    } else {
      fprintf(stderr, "error: unrecognized argument '%s'\n", a);
      return usage(argv[0]);
    }
  }

  if (cfg.nstr <= 0 || cfg.nlyr <= 0 || cfg.nwave <= 0 || cfg.repeat <= 0) {
    fprintf(stderr, "error: --nstr/--nlyr/--nwave/--repeat must be positive\n");
    return 2;
  }

  if (cfg.verify) {
    disort_output sentinel;  // address only; marks "print the fluxes"
    solve_percall(cfg, &sentinel);
    return 0;
  }

  const double seconds =
      cfg.hoist_alloc ? time_hoisted(cfg) : time_percall(cfg);

  // Machine-readable; compare_cdisort.py parses this line.
  printf("SECONDS %.9f\n", seconds);
  return 0;
}
