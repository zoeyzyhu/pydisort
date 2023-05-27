
#include <cdisort/cdisort.h>

/*========================== print_test() ================================*/

/*
 * Print DISORT results and, directly beneath them, their ratios to the
 * correct answers, calc/good, or 1.+calc in the case good = 0.;
 * print number of non-unit ratios that occur but try
 * to count just the cases where there is a real disagreement and not
 * those where flux or intensity are down at their noise level (defined as
 * 10^(-6) times their maximum value).  d(flux)/d(tau) is treated the
 * same as fluxes in this noise estimation even though it is a different
 * type of quantity (although with flux units).
 *
 * Correct input values are "good"; calculated input values are "calc".
 *
 * Fortran name: prtfin().
 */

#undef  BAD_RATIO
#define BAD_RATIO(r) ((r) < 0.99 || (r) > 1.01)

/* Using unit-offset shift macros to match Fortran version */

/* Disort-specific shift macros */
#undef  UMU
#define UMU(iu)    ds_good->umu[iu-1]
#undef  UTAU
#define UTAU(lu)   ds_good->utau[lu-1]

/* Disotest-specific shift macros */
#undef  GOODUU
#define GOODUU(iu,lu,j) good->uu[iu-1+(lu-1+(j-1)*ds_good->ntau)*ds_good->numu]
#undef  CALCUU
#define CALCUU(iu,lu,j) calc->uu[iu-1+(lu-1+(j-1)*ds_calc->ntau)*ds_calc->numu]

void print_test(disort_state  *ds_calc,
                disort_output *calc,
                disort_state  *ds_good,
                disort_output *good)
{
  register int
    iu,j,lu,numbad;
  //extern void
    //  c_errmsg();
  //extern double
    // c_ratio();
  double
    flxmax,umax,fnoise,unoise,
    rat1,rat2,rat3,rat4,
    ratv[ds_good->nphi];

  flxmax = 0.0;
  for (lu = 0; lu < ds_good->ntau; lu++) {
    flxmax = MAX(MAX(MAX(flxmax,good->rad[lu].rfldir),good->rad[lu].rfldn),good->rad[lu].flup);
  }

  fnoise = 1.e-6*flxmax;
  if (flxmax <= 0.) {
    c_errmsg("print_test()--all fluxes zero or negative",DS_WARNING);
  }
  if (fnoise <= 0.) {
    c_errmsg("print_test()--all fluxes near underflowing",DS_WARNING);
  }

  numbad = 0;

  fprintf(stdout,"\n\n                  <-------------- FLUXES -------------->\n"
                 "    Optical       Downward       Downward         Upward    d(Net Flux)\n"
                 "      Depth         Direct        Diffuse        Diffuse    / d(Op Dep)\n");

  for (lu = 1; lu <= ds_good->ntau; lu++) {
    fprintf(stdout,"%11.4f%15.4e%15.4e%15.4e%15.4e\n",
                   UTAU(lu),calc->rad[lu-1].rfldir,calc->rad[lu-1].rfldn,calc->rad[lu-1].flup,calc->rad[lu-1].dfdt);

    fprintf(stdout,"%11.4f%15.4e%15.4e%15.4e%15.4e\n",
                   UTAU(lu),good->rad[lu-1].rfldir,good->rad[lu-1].rfldn,good->rad[lu-1].flup,good->rad[lu-1].dfdt);

    rat1 = c_ratio(calc->rad[lu-1].rfldir,good->rad[lu-1].rfldir);
    rat2 = c_ratio(calc->rad[lu-1].rfldn, good->rad[lu-1].rfldn);
    rat3 = c_ratio(calc->rad[lu-1].flup,  good->rad[lu-1].flup);
    rat4 = c_ratio(calc->rad[lu-1].dfdt,  good->rad[lu-1].dfdt);

    fprintf(stdout,"               (%9.4f)    (%9.4f)    (%9.4f)    (%9.4f)\n",rat1,rat2,rat3,rat4);

    /*
     * NOTE: In the original Fortran, for a/b, ratio() returns a huge number if b == 0., hence
     *       there is an extra conditional of the form fabs(output) > fnoise so that nearly-zero output
     *       will not be counted as bad. In contrast, this C version has ratio() returning 1.+a when b == 0.,
     *       hence the conditional involving fnoise is removed (the same applies to unoise below).
     */
    if(BAD_RATIO(rat1)) numbad++;
    if(BAD_RATIO(rat2)) numbad++;
    if(BAD_RATIO(rat3)) numbad++;
    if(BAD_RATIO(rat4)) numbad++;
  }

  if (!ds_good->flag.onlyfl) {
    /*
     * Print intensities
     */
    umax = 0.;
    for (j = 1; j <= ds_good->nphi; j++) {
      for (lu = 1; lu <= ds_good->ntau; lu++) {
        for (iu = 1; iu <= ds_good->numu; iu++) {
          umax = MAX(umax,GOODUU(iu,lu,j));
        }
      }
    }

    unoise = 1.e-6*umax;

    if (umax <= 0.) {
      c_errmsg("print_test()--all intensities zero or negative",DS_WARNING);
    }

    if (unoise <= 0.) {
      c_errmsg("print_test()--all intensities near underflowing",DS_WARNING);
    }

    fprintf(stdout,"\n\n ********  I N T E N S I T I E S  *********"
                   "\n\n             Polar   Azimuthal Angles (Degrees)"
                   "\n   Optical   Angle"
                   "\n     Depth  Cosine");
    for (j = 0; j < ds_good->nphi; j++) {
      fprintf(stdout,"%10.1f    ",ds_good->phi[j]);
    }
    fprintf(stdout,"\n");

    for (lu = 1; lu <= ds_good->ntau; lu++) {
      for (iu = 1; iu <= ds_good->numu; iu++) {
        if (iu == 1) {
          fprintf(stdout,"\n%10.3f%8.3f",UTAU(lu),UMU(iu));
          for (j = 1; j <= ds_good->nphi; j++) {
            fprintf(stdout,"%14.4e",CALCUU(iu,lu,j));
          }
          fprintf(stdout,"\n");
        }
        if (iu > 1) {
          fprintf(stdout,"          %8.3f",UMU(iu));
          for(j = 1; j <= ds_good->nphi; j++) {
            fprintf(stdout,"%14.4e",CALCUU(iu,lu,j));
          }
          fprintf(stdout,"\n");
        }
        for (j = 1; j <= ds_good->nphi; j++) {
          ratv[j-1] = c_ratio(CALCUU(iu,lu,j),GOODUU(iu,lu,j));
          /*
           * NOTE: This C version has the conditional fabs(output) > unoise removed;
           *       see note above regarding fnoise.
           */
          if(BAD_RATIO(ratv[j-1])) {
            numbad++;
          }
        }
        fprintf(stdout,"                  ");
        for (j = 1; j <= ds_good->nphi; j++) {
          fprintf(stdout,"   (%9.4f)",ratv[j-1]);
        }
        fprintf(stdout,"\n");
      }
    }
  }

  if (numbad > 0) {
    if (numbad == 1) {
      fprintf(stdout,"\n\n =============================================\n"
                     " ====  %4d  SERIOUSLY NON-UNIT RATIO     ====\n"
                     " =============================================\n",numbad);
    }
    else {
      fprintf(stdout,"\n\n =============================================\n"
                     " ====  %4d  SERIOUSLY NON-UNIT RATIOS    ====\n"
                     " =============================================\n",numbad);
    }
  }

  return;
}

/*========================== end of print_test() =========================*/