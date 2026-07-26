/* This file was mechanically split out of cdisort.c (cdisort 2.1.3).
 * Function bodies are unchanged except for the transforms that make them
 * compilable as __host__ __device__ C++ (see cdisort.hpp):
 * DISPATCH_MACRO inline definitions, pmalloc/pcalloc/pfree instead of
 * libc allocation, no 'register', and no function-scope static caches.
 * Include this header only through cdisort.hpp.
 */
#pragma once

#include "cdisort.h"
#include "pmem.h"

/*============================= c_self_test() ===========================*/

/*
 * If  compare is FALSE, set up self-test disort_state ds_test.
 * If  compare is TRUE, compare self-test results with correct
 * answers, abort if error, and free self-test memory.
 *
 * (See file 'DISORT.txt' for variable definitions.)
 *
 *    I N T E R N A L    V A R I A B L E S:
 *
 *       acc        Relative accuracy required for passing self-test
 *       error      Relative errors in DISORT output variables
 *       ok         Logical variable for determining failure of self-test
 *
 * Called by- c_disort
 * Calls- c_errmsg
 */

DISPATCH_MACRO inline void c_self_test(int            compare,
                 int           *prntu0,
                 disort_state  *ds,
                 disort_output *out)
{
  const double
    acc = 1.e-4;
  int
    i,ok;    
  double
    error;

  if(compare == FALSE) {
    for (i = 0; i < 5; i++) {
      ds->flag.prnt[i] = FALSE;
    }
    ds->flag.ibcnd     = GENERAL_BC;
    ds->flag.usrang    = TRUE;
    ds->flag.usrtau    = TRUE;
    ds->flag.lamber    = TRUE;
    ds->flag.onlyfl    = FALSE;
    ds->flag.planck    = TRUE;
    ds->flag.quiet     = QUIET;
    ds->flag.spher     = FALSE;
    ds->flag.general_source = FALSE;
    ds->flag.brdf_type = BRDF_NONE;
    ds->flag.intensity_correction     = TRUE;
    ds->flag.old_intensity_correction = TRUE;
    ds->flag.output_uum=FALSE;

    ds->nstr = 4;
    ds->nlyr = 1;
    ds->nmom = 4;
    ds->numu = 1;
    ds->ntau = 1;
    ds->nphi = 1;

    /* Allocate memory for self test */
    c_disort_state_alloc(ds);
    c_disort_out_alloc(ds,out);

    ds->accur  = 1.e-4;
    ds->wvnmlo =     0.;
    ds->wvnmhi = 50000.;

    ds->bc.fbeam  =  M_PI;
    ds->bc.umu0   =   .866;
    ds->bc.phi0   =   0.;
    ds->bc.fisot  =   1.;
    ds->bc.fluor  =   0.;
    ds->bc.albedo =    .7;
    ds->bc.ttemp  = 100.;
    ds->bc.btemp  = 300.;
    ds->bc.temis  =    .8;

    TEMPER(0) = 210.;
    TEMPER(1) = 200.;

    DTAUC(1)  = 1.;
    SSALB(1)  =  .9;

    /* Haze L moments */
    PMOM(0,1) = 1.;
    PMOM(1,1) =  .8042;
    PMOM(2,1) =  .646094;
    PMOM(3,1) =  .481851;
    PMOM(4,1) =  .359056;

    UMU(1)  =  0.5;
    UTAU(1) =  0.5;
    PHI(1)  = 90.0;

    return;
  }
  else if (compare == TRUE) {
    /*
     * Compare test case results with correct answers and abort if bad
     */
    ok = TRUE;

    error = (out->uu[0]-47.865571)/47.865571;
    if (fabs(error) > acc) {
      ok = FALSE;
      fprintf(stderr,"Output variable uu differed by %g percent from correct value.\n",100.*error);
    }

    error = (out->rad[0].rfldir-1.527286)/1.527286;
    if (fabs(error) > acc) {
      ok = FALSE;
      fprintf(stderr,"Output variable rfldir differed by %g percent from correct value.\n",100.*error);
    }

    error = (out->rad[0].rfldn-28.372225)/28.372225;
    if (fabs(error) > acc) {
      ok = FALSE;
      fprintf(stderr,"Output variable rfldn differed by %g percent from correct value.\n",100.*error);
    }

    error = (out->rad[0].flup-152.585284)/152.585284;
    if (fabs(error) > acc) {
      ok = FALSE;
      fprintf(stderr,"Output variable flup differed by %g percent from correct value.\n",100.*error);
    }

    /* Free allocated memory for self test */
    c_disort_out_free(ds,out);
    c_disort_state_free(ds);

    if (!ok) {
      c_errmsg("DISORT--self-test failed",DS_ERROR);
    }

    return;
  }
  else {
    fprintf(stderr,"**error--self_test(): compare=%d not recognized\n",compare);
    exit(1);
  }
}

/*============================= end of c_self_test() =====================*/

/******************************************************************
 ********** end of DISORT service routines ************************
 ******************************************************************

 ******************************************************************
 ********** ds->flag.ibcnd = SPECIAL_BC routines ******************
 ******************************************************************/

/*============================= c_gaussian_quadrature_test() ============*/

DISPATCH_MACRO inline int c_gaussian_quadrature_test(int nstr, float *sza, double umu0)
{

  /* Test if the solar zenith angle coincides with one of
     the computational angles */

  int nn=0, iq=0, result=0;
  double umu0s=0.0, *cmu=NULL, *cwt=NULL;

  cmu = c_dbl_vector(0,nstr,"cmu");
  if (cmu==NULL) {
    fprintf(stderr,"Error allocating cmu!\n");
    return -1;
  }

  cwt = c_dbl_vector(0,nstr,"cwt");
  if (cwt==NULL) {
    fprintf(stderr,"Error allocating cwt!\n");
    return -1;
  }
    
  nn = nstr / 2.0;

  c_gaussian_quadrature ( nn, cmu, cwt );

  for (iq=1; iq<=nn; iq++) {
    if( fabs( (umu0 - CMU (iq)) / umu0 ) < 1.0e-4 ) {
      umu0s = umu0;
      if ( umu0 < CMU (iq) )
	umu0  = CMU (iq) * (1. - 1.1e-4);
      else
	umu0  = CMU (iq) * (1. + 1.1e-4);

      *sza   = acos (umu0)/DEG;
      fprintf(stderr,"%s %s %s %f %s %f\n",
	      "******* WARNING >>>>>> \n",
	      "SETDIS--beam angle=computational angle;\n",
	      "******* changing cosine of solar zenith angle, umu0, from ",
	      umu0s, "to", umu0 );
      result=-1;
    }
  }

  pfree(cwt);
  pfree(cmu);      
  return result;
}

/*============================= end of c_gaussian_quadrature_test() =====*/

/* * * * * * * * * * * * * * end of cdisort.c * * * * * * * * * * * */

