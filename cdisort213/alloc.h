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

/*============================= c_disort_state_alloc() ==================*/

/*
 * Dynamically allocate memory for disort input arrays, including
 * ones that the user can optionally ask disort() to calculate.
 */
DISPATCH_MACRO inline void c_disort_state_alloc(disort_state *ds)
{
  int
    nu=0;

  ds->fast_flux = FALSE;

  ds->dtauc = c_dbl_vector(0,ds->nlyr,"ds->dtauc");  
  ds->ssalb = c_dbl_vector(0,ds->nlyr,"ds->ssalb");
  /*
   * NOTE: PMOM is used in the code even when ds->nmom is not set by the user
   *       (such as when there is no scattering). Its first dimension needs to be
   *       at least 0:ds->nstr, hence we introduce ds->nmom_nstr in the C version.
   */
  ds->nmom_nstr = IMAX(ds->nmom,ds->nstr);
  ds->pmom      = c_dbl_vector(0,(ds->nmom_nstr+1)*ds->nlyr-1,"ds->pmom");

  if (ds->flag.ibcnd == SPECIAL_BC) {
    ds->flag.planck = FALSE;
    ds->flag.lamber = TRUE;
    ds->flag.usrtau = FALSE;
  }

  /* range 0 to nlyr */
  if (ds->flag.planck == TRUE) {
    ds->temper = c_dbl_vector(0,ds->nlyr,"ds->temper");
  }
  else {
    ds->temper = NULL;
  }

  if (ds->flag.general_source == TRUE) {
    ds->gensrc  = c_dbl_vector(0,ds->nstr*ds->nlyr*ds->nstr,"ds->gensrc");
    ds->gensrcu = c_dbl_vector(0,ds->nstr*ds->nlyr*ds->numu,"ds->gensrcu");
  }
  else {
    ds->gensrc  = NULL;
    ds->gensrcu = NULL;
  }

  if (ds->flag.usrtau == FALSE) {
    ds->ntau = ds->nlyr+1;
  }
  ds->utau = c_dbl_vector(0,ds->ntau-1,"ds->utau");

  //20130723ak Treat as in c_twostr_state_alloc. See comment there.
  //20130723ak Thanks to Tim for reporting this one.
  ds->zd        = c_dbl_vector(0,ds->nlyr+1,"ds->zd");

  /* range starts at 0 */
  nu = ds->numu;
  if ( (!ds->flag.usrang || ds->flag.onlyfl)) nu = ds->nstr;

  if (ds->flag.ibcnd == SPECIAL_BC) 
    ds->umu = c_dbl_vector(0,2*nu,"ds->umu");
  else
    ds->umu = c_dbl_vector(0,nu,"ds->umu");

  if (ds->nphi >= 1) {
    ds->phi = c_dbl_vector(0,ds->nphi-1,"ds->nphi");
  }
  else {
    ds->phi = NULL;
  }

  if (!ds->flag.old_intensity_correction) {
    if (ds->nphase >= 1) {
      ds->mu_phase = c_dbl_vector(0,ds->nphase-1,"ds->mu_phase");
      ds->phase = c_dbl_vector(0,ds->nlyr*ds->nphase-1,"ds->phase");
    }
    else {
      ds->mu_phase = NULL;
      ds->phase = NULL;
    }
  }

  switch(ds->flag.brdf_type) {
    case BRDF_RPV:
      ds->brdf.rpv = (rpv_brdf_spec *)pcalloc(1,sizeof(rpv_brdf_spec));
      if (!ds->brdf.rpv) {
        c_errmsg("calloc error for ds->brdf.rpv",DS_ERROR);
      }
    break;
#if HAVE_BRDF
    case BRDF_AMB:
      ds->brdf.ambrals = (ambrals_brdf_spec *)pcalloc(1,sizeof(ambrals_brdf_spec));
      if (!ds->brdf.ambrals) {
        c_errmsg("calloc error for ds->brdf.ambrals",DS_ERROR);
      }
    break;
    case BRDF_CAM:
      ds->brdf.cam = (cam_brdf_spec *)pcalloc(1,sizeof(cam_brdf_spec));
      if (!ds->brdf.cam) {
        c_errmsg("calloc error for ds->brdf.cam",DS_ERROR);
      }
    break;
#endif
    default:
      ;
    break;
  }

  return;
}

/*============================= end of c_disort_state_alloc() ============*/

/*============================= c_disort_state_free() ===================*/

/*
 *  Free memory allocated by disort_state_alloc()
 */
DISPATCH_MACRO inline void c_disort_state_free(disort_state *ds)
{
  if (ds->phi)    pfree(ds->phi);
  if (ds->umu)    pfree(ds->umu);
  if (ds->utau)   pfree(ds->utau);
  if (ds->temper) pfree(ds->temper);
  if (ds->pmom)   pfree(ds->pmom);
  if (ds->ssalb)  pfree(ds->ssalb);
  if (ds->dtauc)  pfree(ds->dtauc);
  if (ds->zd)     pfree(ds->zd);
  if (ds->flag.general_source == TRUE) {
    if (ds->gensrc)     pfree(ds->gensrc);
    if (ds->gensrcu)    pfree(ds->gensrcu);
  }
  if (!ds->flag.old_intensity_correction) {
    if (ds->nphase >= 1) {
      pfree(ds->mu_phase);
      pfree(ds->phase);
    }
  }

  switch(ds->flag.brdf_type) {
    case BRDF_RPV:
      pfree(ds->brdf.rpv);
    break;
#if HAVE_BRDF
    case BRDF_AMB:
      pfree(ds->brdf.ambrals);
    break;
    case BRDF_CAM:
      pfree(ds->brdf.cam);
    break;
#endif
    default:
      ;
    break;
  }

  return;
}

/*============================= end of c_disort_state_free() ============*/

/*============================= c_disort_out_alloc() ====================*/

/*
 *   Dynamically allocate memory for disort output arrays
 */
DISPATCH_MACRO inline void c_disort_out_alloc(disort_state  *ds,
                        disort_output *out)
{

  int
    nu;

  out->rad = (disort_radiant *)pcalloc(ds->ntau,sizeof(disort_radiant));

  if (!out->rad) {
    c_errmsg("disort_out_alloc---error allocating out->rad array",DS_ERROR);
  }
  nu = ds->numu;
  if ( (!ds->flag.usrang || ds->flag.onlyfl)) {
    nu = ds->nstr;
  }
#ifdef __CUDA_ARCH__
  if (ds->flag.onlyfl) {
    out->uu = NULL;
    out->u0u = NULL;
  }
  else
#endif
  {
    out->uu = c_dbl_vector(0,ds->nphi*nu*ds->ntau,"out->uu");
    out->u0u = c_dbl_vector(0,ds->ntau*nu,"out->u0u");
  }

  if ( ds->flag.output_uum )
    out->uum = c_dbl_vector(0,ds->nstr*nu*ds->ntau,"out->uum");

  if (ds->flag.ibcnd == SPECIAL_BC) {
    out->albmed = c_dbl_vector(0,ds->numu,"out->albmed");
    out->trnmed = c_dbl_vector(0,ds->numu,"out->trnmed");
  }
  else {
    out->albmed = NULL;
    out->trnmed = NULL;
  }

  return;
}

/*============================= end of c_disort_out_alloc() =============*/

/*============================= c_disort_out_free() =====================*/

/*
 * Free memory allocated by disort_out_alloc()
 */
DISPATCH_MACRO inline void c_disort_out_free(disort_state  *ds,
                       disort_output *out)
{

  if (out->trnmed) pfree(out->trnmed);
  if (out->albmed) pfree(out->albmed);
  if (out->u0u)    pfree(out->u0u);
  if (out->uu)     pfree(out->uu);
  if (out->rad)    pfree(out->rad);
  if ( ds->flag.output_uum ) 
    if (out->uum) pfree(out->uum);

  return;
}

/*============================= end of c_disort_out_free() ==============*/

/*============================= c_twostr_state_alloc() ==================*/

/*
 * Dynamically allocate memory for twostr input arrays.
 */
DISPATCH_MACRO inline void c_twostr_state_alloc(disort_state *ds)
{
  /* Set to two streams */
  ds->nstr = 2;

  /* Set flags not controlled by user */
  ds->flag.prnt[2] = FALSE;
  ds->flag.prnt[3] = FALSE;
  ds->flag.prnt[4] = FALSE;
  ds->flag.onlyfl  = TRUE;

  ds->dtauc = c_dbl_vector(0,ds->nlyr-1,"ds->dtauc");
  ds->ssalb = c_dbl_vector(0,ds->nlyr-1,"ds->ssalb");

  /* range 0 to nlyr */
  if (ds->flag.planck == TRUE) {
    ds->temper = c_dbl_vector(0,ds->nlyr,"ds->temper");
  }
  else {
    ds->temper = NULL;
  }

  if (ds->flag.usrtau == FALSE) {
    ds->ntau = ds->nlyr+1;
  }
  ds->utau = c_dbl_vector(0,ds->ntau-1,"ds->utau");

  //20120820ak Tim says: if spher is false
  //20120820ak during allocation, it has a seg fault later if you turn spher
  //20120820ak on and try to use that functionality.  Probably would be
  //20120820ak better to just allocate this little guy regardless of the status of spher.
  //20120820ak So I commented the following.
  //20120820ak  if (ds->flag.spher == TRUE) {
  ds->zd        = c_dbl_vector(0,ds->nlyr+1,"ds->zd");	
  //20120820ak  }

  return;
}

/*============================= endof c_twostr_state_alloc() ============*/

/*============================= c_twostr_state_free() ===================*/

/*
 *  Free memory allocated by twostr_state_alloc()
 */
DISPATCH_MACRO inline void c_twostr_state_free(disort_state *ds)
{
  if (ds->utau)   pfree(ds->utau);
  if (ds->temper) pfree(ds->temper);
  if (ds->ssalb ) pfree(ds->ssalb);
  if (ds->dtauc ) pfree(ds->dtauc);
  if (ds->zd )    pfree(ds->zd);
  return;
}

/*============================= end of c_twostr_state_free() ============*/

/*============================= c_twostr_out_alloc() ====================*/

/*
 *   Dynamically allocate memory for twostr output arrays
 */
DISPATCH_MACRO inline void c_twostr_out_alloc(disort_state  *ds,
                        disort_output *out)
{
  out->rad = (disort_radiant *)pcalloc(ds->ntau,sizeof(disort_radiant));
  if (!out->rad) {
    c_errmsg("disort_out_alloc---error allocating out->rad array",DS_ERROR);
  }

  return;
}

/*============================= end of c_twostr_out_alloc() =============*/

/*============================= c_twostr_out_free() =====================*/

/*
 * Free memory allocated by twostr_out_alloc()
 */
DISPATCH_MACRO inline void c_twostr_out_free(disort_state  *ds,
                       disort_output *out)
{
  if (out->rad) pfree(out->rad);

  return;
}

/*============================= end of c_twostr_out_free() ==============*/

/*============================= c_dbl_vector() ==========================*/

/*
 * Allocates memory for a 1D double-precision array with range [nl..nh].
 *
 * NOTE: calloc() zeros the memory it allocates.
 */ 

DISPATCH_MACRO inline double *c_dbl_vector(int  nl, 
		     int  nh,
		     char const *name)
{
  unsigned int  
    len_safe;
  int           
    nl_safe, nh_safe;
  double         
    *m;

  if (nh < nl) {
    fprintf(stderr,"\n\n**error:%s, variable %s, range (%d,%d)\n","dbl_vector",name,nl,nh);
    exit(1);
  }

  nl_safe  = (nl < 0) ? nl : 0;
  nh_safe  = (nh > 0) ? nh : 0;
  len_safe = (unsigned)(nh_safe-nl_safe+1);

  m = (double *)pcalloc(len_safe,sizeof(double));

  if (!m) {
    c_errmsg("dbl_vector---alloc error",DS_ERROR);
  }
  m -= nl_safe;

  return m;
}

/*============================= end of c_dbl_vector() ===================*/

/*
 * Allocates a 1D double-precision array with range [nl..nh] without
 * initializing it.  CUDA flux-only calls use this only for buffers that are
 * cleared unconditionally before their first read.
 */
DISPATCH_MACRO inline double *c_dbl_vector_uninitialized(int nl,
                                                          int nh,
                                                          char const *name)
{
  unsigned int len_safe;
  int nl_safe, nh_safe;
  double *m;

  if (nh < nl) {
    fprintf(stderr,"\n\n**error:%s, variable %s, range (%d,%d)\n",
            "dbl_vector",name,nl,nh);
    exit(1);
  }

  nl_safe = (nl < 0) ? nl : 0;
  nh_safe = (nh > 0) ? nh : 0;
  len_safe = (unsigned)(nh_safe-nl_safe+1);
  m = (double *)pmalloc(len_safe*sizeof(double));

  if (!m) {
    c_errmsg("dbl_vector---alloc error",DS_ERROR);
  }
  return m - nl_safe;
}

/*============================= c_int_vector() ==========================*/

/*
 * Allocates memory for a 1D integer array with range [nl..nh].
 *
 * NOTE: calloc() zeros the memory it allocates.
 */ 

DISPATCH_MACRO inline int *c_int_vector(int  nl, 
		  int  nh,
		  char const *name)
{
  unsigned int  
    len_safe;
  int           
    nl_safe, nh_safe;
  int         
    *m;

  if (nh < nl) {
    fprintf(stderr,"\n\n**error:%s, variable %s, range (%d,%d)\n","int_vector",name,nl,nh);
    exit(1);
  }

  nl_safe  = (nl < 0) ? nl : 0;
  nh_safe  = (nh > 0) ? nh : 0;
  len_safe = (unsigned)(nh_safe-nl_safe+1);

  m = (int *)pcalloc(len_safe,sizeof(int));

  if (!m) {
    c_errmsg("int_vector---alloc error",DS_ERROR);
  }
  m -= nl_safe;

  return m;
}

/*============================= end of c_int_vector() ===================*/

/*============================= c_free_dbl_vector() =====================*/

/*
 * Frees memory allocated by dbl_vector().
 *
 * NOTE: If the array is zero-offset, can just use free().
 * NOTE: Argument nh is not used, but kept to match dbl_vector().
 */

DISPATCH_MACRO inline void c_free_dbl_vector(double *m, 
                       int     nl, 
                       int     nh)
{
  int  
    nl_safe;

  nl_safe = (nl < 0) ? nl : 0;
  m      += nl_safe;
  pfree(m);

  return;
}

/*============================= end of c_free_dbl_vector() ==============*/
