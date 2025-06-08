/******************************************************************************
 * test_cdisort_09.c
 *
 * A standalone driver for “Test Problem 09” from the original test suite.
 *
 * Increased the number of streams to 32 and the number of layers to 100.
 * Looping through 1000 wavenumbers to simulate a longer run.
 * This test is designed to validate the performance under increased load.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "cdisort.h"

#undef DTAUC
#define DTAUC(lc) ds.dtauc[lc - 1]
#undef PHI
#define PHI(j) ds.phi[j - 1]
#undef PMOM
#define PMOM(k, lc) ds.pmom[k + (lc - 1) * (ds.nmom_nstr + 1)]
#undef SSALB
#define SSALB(lc) ds.ssalb[lc - 1]
#undef TEMPER
#define TEMPER(lc) ds.temper[lc]
#undef UMU
#define UMU(iu) ds.umu[iu - 1]
#undef UTAU
#define UTAU(lu) ds.utau[lu - 1]

/*
 * Disotest-specific shift macros
 */
#undef GOODUU
#define GOODUU(iu, lu, j) \
  good.uu[iu - 1 + (lu - 1 + (j - 1) * ds.ntau) * ds.numu]

/*=============================================================================
 * main: run only test 09
 *===========================================================================*/
int main(void) {
  printf("Running DISORT test 09...\n\n");
  for (int i = 0; i < 500; i++) {
    // Simulate run for 1000 wavenumbers
    disort_test09();
  }
  printf("\nTest 09 completed.\n");
  return 0;
}

/*=============================================================================
 * BEGIN copy of disort_test09() from tests/cdisort213/test_cdisort.c
 *===========================================================================*/

void disort_test09(void) {
  register int icas, lc, k;
  const int ncase = 1;
  double gg;
  disort_state ds;
  disort_output out, good;

  ds.accur = 0.;
  // Modified to no printing
  ds.flag.prnt[0] = FALSE, ds.flag.prnt[1] = FALSE, ds.flag.prnt[2] = FALSE,
  ds.flag.prnt[3] = FALSE, ds.flag.prnt[4] = FALSE;

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

  ds.nstr = 32;   // originally 8
  ds.nlyr = 100;  // originally 6
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

  for (icas = 1; icas <= ncase; icas++) {
    switch (icas) {
      case 1:
        ds.flag.planck = FALSE;

        /* Allocate memory */
        c_disort_state_alloc(&ds);
        c_disort_out_alloc(&ds, &out);
        c_disort_out_alloc(&ds, &good);

        for (lc = 1; lc <= ds.nlyr; lc++) {
          DTAUC(lc) = (double)lc / ds.nlyr * 6;  // originally (double)lc
          SSALB(lc) =
              0.6 + (double)lc * 0.003;  // originally 0.6+(double)lc*0.05
        }

        UTAU(1) = 0.;
        UTAU(2) = 1.05;
        UTAU(3) = 2.1;
        UTAU(4) = 6.;
        UTAU(5) = 21.;

        UMU(1) = -1.;
        UMU(2) = -0.2;
        UMU(3) = 0.2;
        UMU(4) = 1.;

        PHI(1) = 60.;

        for (lc = 1; lc <= ds.nlyr; lc++) {
          c_getmom(ISOTROPIC, 0., ds.nmom, &PMOM(0, lc));
        }

        ds.bc.albedo = 0.;
        // sprintf(ds.header,"Test Case No. 9a:  Ref. DGIS, Tables VI-VII,
        // beta=l=0 (multiple inhomogeneous layers)");

        /* Correct answers */
        good.rad[0].rfldir = 0., good.rad[1].rfldir = 0.,
        good.rad[2].rfldir = 0., good.rad[3].rfldir = 0.,
        good.rad[4].rfldir = 0.;
        good.rad[0].rfldn = 1., good.rad[1].rfldn = 3.55151E-01,
        good.rad[2].rfldn = 1.44265E-01, good.rad[3].rfldn = 6.71445E-03,
        good.rad[4].rfldn = 6.16968E-07;
        good.rad[0].flup = 2.27973E-01, good.rad[1].flup = 8.75098E-02,
        good.rad[2].flup = 3.61819E-02, good.rad[3].flup = 2.19291E-03,
        good.rad[4].flup = 0.;
        good.rad[0].dfdt = 8.82116E-01, good.rad[1].dfdt = 2.32366E-01,
        good.rad[2].dfdt = 9.33443E-02, good.rad[3].dfdt = 3.92782E-03,
        good.rad[4].dfdt = 1.02500E-07;
        GOODUU(1, 1,
               1) = 3.18310E-01,
               GOODUU(2, 1, 1) = 3.18310E-01, GOODUU(3, 1, 1) = 9.98915E-02,
               GOODUU(4, 1, 1) = 5.91345E-02, GOODUU(1, 2, 1) = 1.53507E-01,
               GOODUU(2, 2, 1) = 5.09531E-02, GOODUU(3, 2, 1) = 3.67006E-02,
               GOODUU(4, 2, 1) = 2.31903E-02, GOODUU(1, 3, 1) = 7.06614E-02,
               GOODUU(2, 3, 1) = 2.09119E-02, GOODUU(3, 3, 1) = 1.48545E-02,
               GOODUU(4, 3, 1) = 9.72307E-03, GOODUU(1, 4, 1) = 3.72784E-03,
               GOODUU(2, 4, 1) = 1.08815E-03, GOODUU(3, 4, 1) = 8.83316E-04,
               GOODUU(4, 4, 1) = 5.94743E-04, GOODUU(1, 5, 1) = 2.87656E-07,
               GOODUU(2, 5, 1) = 1.05921E-07, GOODUU(3, 5, 1) = 0.,
               GOODUU(4, 5, 1) = 0.;
        break;
    }

    c_disort(&ds, &out, c_planck_func2);

    // print_test(&ds,&out,&ds,&good);

    /* Free allocated memory */
    c_disort_out_free(&ds, &good);
    c_disort_out_free(&ds, &out);
    c_disort_state_free(&ds);
  }

  return;
}
