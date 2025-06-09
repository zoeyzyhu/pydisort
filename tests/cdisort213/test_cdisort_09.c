/******************************************************************************
 * test_cdisort_09.c
 *
 * A standalone driver for “Test Problem 09” from the original test suite,
 * with command-line control over the number of streams (nstr) and layers
 *(nlyr).
 *
 * Usage:
 *   test_cdisort_09 [nstr nlyr nwave]
 *   Defaults: nstr = 32, nlyr = 100, nwave = 1000
 *
 * Loops through nwave wavenumbers to simulate a longer run.
 *****************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "cdisort.h"

#undef DTAUC
#define DTAUC(lc) ds.dtauc[(lc) - 1]
#undef PHI
#define PHI(j) ds.phi[(j) - 1]
#undef PMOM
#define PMOM(k, lc) ds.pmom[(k) + ((lc) - 1) * (ds.nmom_nstr + 1)]
#undef SSALB
#define SSALB(lc) ds.ssalb[(lc) - 1]
#undef TEMPER
#define TEMPER(lc) ds.temper[(lc)]
#undef UMU
#define UMU(iu) ds.umu[(iu) - 1]
#undef UTAU
#define UTAU(lu) ds.utau[(lu) - 1]

#undef GOODUU
#define GOODUU(iu, lu, j) \
  good.uu[(iu) - 1 + (((lu) - 1 + ((j) - 1) * ds.ntau) * ds.numu)]

void run_disort_test09(int nstr, int nlyr, double ssalb);

int main(int argc, char **argv) {
  int nstr = 32;
  int nlyr = 100;
  int nwave = 1000;  // Number of wavenumbers to loop through
  double ssalb = 0.003;

  if (argc >= 3) {
    nstr = atoi(argv[1]);
    nlyr = atoi(argv[2]);
    nwave = atoi(argv[3]);
    ssalb = atof(argv[4]);
  } else {
    printf("Usage: %s [nstr nlyr] (default %d %d)\n", argv[0], nstr, nlyr);
  }

  printf("Running DISORT test 09 with nstr=%d, nlyr=%d, nwave=%d, ssalb=%f\n\n",
         nstr, nlyr, nwave, ssalb);

  for (int i = 0; i < nwave; ++i) {
    run_disort_test09(nstr, nlyr, ssalb);
  }

  printf("\nTest 09 completed.\n");
  return 0;
}

void run_disort_test09(int nstr, int nlyr, double ssalb) {
  register int icas, lc, k;
  const int ncase = 1;
  double gg;
  disort_state ds;
  disort_output out, good;

  /* Initialize flags */
  ds.accur = 0.;
  ds.flag.prnt[0] = FALSE;
  ds.flag.prnt[1] = FALSE;
  ds.flag.prnt[2] = FALSE;
  ds.flag.prnt[3] = FALSE;
  ds.flag.prnt[4] = FALSE;

  ds.flag.ibcnd = GENERAL_BC;
  ds.flag.usrtau = TRUE;
  ds.flag.usrang = TRUE;
  ds.flag.lamber = TRUE;
  ds.flag.onlyfl = FALSE;
  ds.flag.quiet = TRUE;
  ds.flag.spher = FALSE;
  ds.flag.general_source = FALSE;
  ds.flag.output_uum = FALSE;
  ds.flag.intensity_correction = TRUE;
  ds.flag.old_intensity_correction = TRUE;

  /* Apply user-specified geometry */
  ds.nstr = nstr;
  ds.nlyr = nlyr;
  ds.nphase = ds.nstr;
  ds.nmom = ds.nstr;
  ds.ntau = 5;
  ds.numu = 4;
  ds.nphi = 1;

  ds.bc.fbeam = 0.;
  ds.bc.fisot = 1. / M_PI;
  ds.bc.phi0 = 0.0;
  ds.bc.umu0 = 0.5;
  ds.bc.fluor = 0.;

  ds.flag.brdf_type = BRDF_NONE;

  for (icas = 1; icas <= ncase; ++icas) {
    switch (icas) {
      case 1:
        ds.flag.planck = FALSE;

        /* Allocate memory */
        c_disort_state_alloc(&ds);
        c_disort_out_alloc(&ds, &out);
        c_disort_out_alloc(&ds, &good);

        /* Set optical properties per layer */
        for (lc = 1; lc <= ds.nlyr; ++lc) {
          DTAUC(lc) = ((double)lc / ds.nlyr) * 6;
          SSALB(lc) = 0.6 + (double)lc * ssalb;
        }

        /* Tau grid" (fixed 5 points) */
        UTAU(1) = 0.;
        UTAU(2) = 1.05;
        UTAU(3) = 2.1;
        UTAU(4) = 6.;
        UTAU(5) = 21.;

        /* Cosine angles */
        UMU(1) = -1.;
        UMU(2) = -0.2;
        UMU(3) = 0.2;
        UMU(4) = 1.;

        /* Azimuthal angle */
        PHI(1) = 60.;

        /* Phase function moments */
        for (lc = 1; lc <= ds.nlyr; ++lc) {
          c_getmom(ISOTROPIC, 0., ds.nmom, &PMOM(0, lc));
        }

        ds.bc.albedo = 0.;

        break;
    } /* Execute DISORT with Planck emission function */
    c_disort(&ds, &out, c_planck_func2);

    /* Clean up */
    c_disort_out_free(&ds, &good);
    c_disort_out_free(&ds, &out);
    c_disort_state_free(&ds);
  }
  return;
}
