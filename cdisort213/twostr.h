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

/*
 * Shift macros that are different than for disort
 * (restored to the cdisort.h definition at the end of this header)
 */
#undef  KK
#define KK(lyu) kk[lyu-1]

/*============================= c_twostr() ===============================*/

/*
 Copyright (C) 1993, 1994, 1995 Arve Kylling

 C rewrite by Timothy E. Dowling (Univ. of Louisville)

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 1, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 To obtain a copy of the GNU General Public License write to the
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 USA.

+---------------------------------------------------------------------+

     AUTHOR :  Arve Kylling (July 1993)
               Arve.Kylling@itek.norut.no

     REFERENCES (cited in the programs using the acronyms shown):

     DS: Dahlback, A. and K. Stamnes 1991: A new spherical
      model for computing the radiation field available
      for photolysis and heating at twilight, Planet.
      Space Sci. 39, 671-683.

     KS: Kylling, A., and K. Stamnes 1992: Efficient yet accurate
      solution of the linear transport  equation in the 
      presence of internal sources: the exponential-linear
      approximation, J. Comp. Phys. 102, 265-276.

    KST: Kylling, A., K. Stamnes and S.-C. Tsay 1995: A reliable
      and efficient two-stream algorithm for radiative
      transfer; Documentation of accuracy in realistic
      layered media, in print, Journal of Atmospheric
      Chemistry 21, 115-150.

    STWJ: Stamnes, K., S.-C. Tsay, W. Wiscombe and K. Jayaweera
      1988: Numerically stable algorithm for discrete-
      ordinate-method radiative transfer in multiple
      scattering and emitting layered media, Appl.
      Optics., 27, 2502.

    WW: Wiscombe, W., 1977:  The Delta-M Method: Rapid Yet
      Accurate Radiative Flux Calculations, J. Atmos. Sci.
      34, 1408-1422

+---------------------------------------------------------------------+

    I n t r o d u c t o r y    n o t e

    (References are given as author-last-name strings, e.g., KST.)

    twostr() solves the radiative transfer equation in an absorbing,
    emitting and multiple scattering, layered pseudo-spherical
    medium in the two-stream approximation. For a discussion of the
    theory behind the present implementation see (KST).

    twostr() is based on the general n-stream algorithm DISORT 
    described in Stamnes et al. (1988, STWJ), and incorporates 
    all the advanced features of that algorithm. Furthermore it
    has been extended to include spherical geometry using the 
    perturbation approach of Dahlback and Stamnes (1991). Relative
    to DISORT, it is both simplified and extended as follows:

     1) The only quantities calculated are mean intensities and fluxes.

     2) The medium may be taken to be pseudo-spherical (flag.spher is TRUE)

     3) Only Lambertian reflection at the bottom boundary is allowed


    General remarks about the structure of the input/output parameters

    The list of input variables is more easily comprehended if
    the following simple facts are borne in mind :

    * there is one vertical coordinate, measured in optical depth units;

    * the layers necessary for computational purposes are entirely
      decoupled from the levels at which the user desires results.

    The computational layering is usually constrained by the problem,
    in the sense that each computational layer must be reasonably
    homogeneous and not have a temperature variation of more than
    about 20 K across it (if thermal sources are considered).
    For example, a clear layer overlain by a cloud overlain by a
    dusty layer would suggest three computational layers.

    However the radiant quantities can be returned to the user at ANY
    level.  For example, the user may have picked 3 computational 
    layers, but he can then request intensities from e.g. only the
    middle of the 2nd layer.

+---------------------------------------------------------------------+

    I n p u t    v a r i a b l e s

    Note on units:

       The radiant output units are determined by the sources of
    radiation driving the problem.  Lacking thermal emission, the
    radiant output units are the units of the sources ds.bc.fbeam and
    ds.bc.fisot.
       If thermal emission of any kind is included, subprogram planck_func2()
    determines the units.  The default planck_func2() has mks units [w/sq m].
    ds.bc.fbeam and ds.bc.fisot must have the same units as planck_func2() when
    thermal emission is present.


    ********  Computational layer structure  ********

        ===========================================================
        == Note:  Layers are numbered from the top boundary down ==
        ===========================================================

    ds.nlyr     Number of computational layers

    DTAUC(lc)   lc = 1 to ds.nlyr,
                optical depths of computational layers

    SSALB(lc)   lc = 1 to ds.nlyr,
                single-scatter albedos of computational layers

    GG(lc)      lc = 1 to ds.nlyr,
                asymmetry factor of computational layers
                Should be <= 1.0 (complete forward scattering) and
                >= -1.0 (complete backward scattering).
                NOTE. GG is changed by twostr() if deltam = TRUE.

    TEMPER(lev) lev = 0 to ds.nlyr, temperatures [K] of levels.
                (Note that temperature is specified at levels
                rather than for layers.)  Don't forget to put top
                temperature in 'TEMPER(0)', not 'TEMPER(1)'.  Top and
                bottom values do not need to agree with top and
                bottom boundary temperatures ds.bc.ttemp and ds.bc.btemp 
                (i.e. slips are allowed).
                Needed only if ds.flag.planck is TRUE.

    ZD(lev)     lev = 0 to ds.nlyr, altitude of level above
                the ground, i.e. ZD(nlyr) = 0., the surface of 
                the planet. Typically in units of (km)
                Must have same units as -radius-. 
                Used to calculate the Chapman function when
                spherical geometry is needed. 
                Needed only if flag.spher is TRUE.

    ds.wvnmlo,  Wavenumbers (inv cm) of spectral interval
      ds.wvnmhi ( used only for calculating Planck function )
                needed only if ds.flag.planck is true.
                If ds.wvnmlo < ds.wvnmhi the Planck function is
                integrated over this interval. If ds.wvnmlo ==  ds.wvnmhi
                the Planck function at wvnmlo is returned.


    ********  User level organization  ********

    ds.flag.usrtau = FALSE, radiant quantities are to be returned
                     at boundary of every computational layer.

                   = TRUE,  radiant quantities are to be returned
                     at user-specified optical depths, as follows:

    ds.ntau        Number of optical depths

    UTAU(lu)       lu = 1 to ds.ntau, user optical depths, in increasing order.
                   UTAU(ntau) must be no greater than the total optical depth of the medium.

     ******** Top and bottom boundary conditions  *********

    ds.bc.fbeam : Intensity of incident parallel beam at top boundary.
                  (units w/sq m if thermal sources active, otherwise
                  arbitrary units).  Corresponding incident flux
                  is  'umu0'  times 'fbeam'.  Note that this is an
                  infinitely wide beam, not a searchlight beam.

    ds.bc.umu0  : Polar angle cosine of incident beam.

    ds.bc.fisot : Intensity of top-boundary isotropic illumination.
                  (units w/sq m if thermal sources active, otherwise
                  arbitrary units).  Corresponding incident flux
                  is  pi (M_PI = 3.14159...)  times 'fisot'.

    ds.bc.albedo: Bottom-boundary albedo, bottom boundary is
                  assumed to be Lambert reflecting.

    ds.bc.btemp : Temperature of bottom boundary (K)  (bottom
                  emissivity is calculated from -albedo-,
                  so it need not be specified).
                  Needed only if -planck- is true.

    ds.bc.ttemp : Temperature of top boundary (K)
                  Needed only if -planck- is true.

    ds.bc.temis : Emissivity of top boundary
                  Needed only if -planck- is true.

    radius      : Distance from center of planet to the planets 
                  surface (km) 

    **********  Control flags  **************

    ds.flag.planck  = TRUE, include thermal emission
                      FALSE, ignore all thermal emission (saves computer time)
                     ( If ds.flag.planck = FALSE, it is not necessary to set any of
                      the variables having to do with thermal emission )

    ds.flag.prnt[0] = TRUE, print input variables 
    ds.flag.prnt[1] = TRUE, print fluxes, mean intensities and flux divergence.

    deltam  = TRUE,  use delta-m method ( see Wiscombe, 1977 )
            = FALSE, don't use delta-m method
            In general intensities and fluxes will be more accurate
            for phase functions with a large forward peak (i.e.
            an asymmetry factor close to 1.) if 'deltam' is set true.

    ds.flag.spher = TRUE, spherical geometry accounted for. In this case
                    -radius- and -zd- must be set also. NOTE: this option
                    increases the execution time, hence use it only when
                    necessary if speed is of concern.
                  = FALSE, plane-parallel atmosphere assumed

    ds.header   : A 127- (or less) character header for prints


+---------------------------------------------------------------------+
               O u t p u t    v a r i a b l e s
+---------------------------------------------------------------------+

    == Note on units == If thermal sources are specified, fluxes come
                        out in [w/sq m] and intensities in [w/sq m/steradian].  
                        Otherwise, the flux and intensity units are determined
                        by the units of -fbeam- and -fisot-.

    If ds.flag.usrtau = FALSE :

         ds.ntau      Number of optical depths at which radiant
                      quantities are evaluated ( = nlyr+1 )

         UTAU(lu)     lu = 1 to ntau, optical depths, in increasing
                      order, corresponding to boundaries of
                      computational layers (see -dtauc-)

    RFLDIR(lu)    :   Direct-beam flux (without delta-m scaling)

    RFLDN(lu)     :   Diffuse down-flux (total minus direct-beam)
                      (without delta-m scaling)

    FLUP(lu)      :   Diffuse up-flux

    DFDT(lu)      :   Flux divergence  d(net flux)/d(optical depth),
                      where 'net flux' includes the direct beam
                      (an exact result;  not from differencing fluxes)

    UAVG(lu)      :   Mean intensity (including the direct beam)

    IERROR(i)     :   Error flag array, if IERROR(i) is zero everything
                      is ok, otherwise twostr() found a fatal error, in this
                      case, twostr return immediately and reports the error in IERROR.

                      i =  1 : ds.nlyr <  1

                      i =  3 : dtauc   <  0.
                      i =  4 : ssalb   <  0. || ssalb > 1.
                      i =  5 : temper  <  0. 
                      i =  6 : gg      < -1. || gg > 1.
                      i =  7 : ZD(lc)  >  ZD(lc-1)
                      i =  8 : ds.ntau <  1

                      i = 10 : UTAU(lu) < 0. || UTAU(lu) > TAUC(nlyr) 

                      i = 12 : fbeam    < 0.
                      i = 13 : if flag.spher = FALSE
                                    umu0  < 0. || umu0 > 1.
                               if flag.spher = TRUE
                                    umu0  < 0. || umu0 > 1.
                      i = 14 : fisot   < 0.
                      i = 15 : albedo  < 0. || albedo > 1.
                      i = 16 : wvnmlo  < 0. || wvnmhi < wvnmlo
                      i = 17 : temis   < 0. || temis  > 1.
                      i = 18 : btemp   < 0.
                      i = 19 : ttemp   < 0.

                      i = 22 : !ds->flag.usrtau && ds->ntau < ds->nlyr+1

                     NOTE: i = 2, 9, 11, 20, and 21 are eliminated in the C version by the
                           change from static to dynamic memory allocation

+---------------------------------------------------------------------+

                 I/O variable specifications

+---------------------------------------------------------------------+
      Routines called (in order): c_twostr_check_inputs, c_twostr_set, c_twostr_print_inputs,
                                  c_twostr_solns, c_set_matrix, c_twostr_solve_bc, c_twostr_fluxes
+---------------------------------------------------------------------+

  Index conventions (for all loops and all variable descriptions):

     iq     :  For quadrature angles
     lu     :  For user levels
     lc     :  For computational layers (each having a different single-scatter albedo and/or phase function)
     lev    :  For computational levels
     ls     :  Runs from 0 to 2*ds->nlyr+1, ls = 1,2,3 refers to top, center and bottom of layer 1, 
               ls = 3,4,5 refers to top, center and bottom of layer 2, etc.

+---------------------------------------------------------------------+

               I n t e r n a l    v a r i a b l e s

   B()...........Right-hand side vector of eqs. KST(38-41), set in twostr_solve_bc()
   bplanck.......Intensity emitted from bottom boundary
   CBAND().......Matrix of left-hand side of the linear system eqs. KST(38-41); in tridiagonal form
   CH(lc)........The Chapman-factor to correct for pseudo-spherical geometry in the direct beam.
   CHTAU(lc).....The optical depth in spherical geometry.
   cmu...........Computational polar angle, single or double Gaussian quadrature rule used, see twostr_set()
   EXPBEA(lc)....Transmission of direct beam in delta-m optical depth coordinates
   FLDIR(lu).....Direct beam flux (delta-m scaled); fl[].zero (see cdisort.h)
   FLDN(lu)......Diffuse down flux (delta-m scaled); fl[].one (see cdisort.h)
   FLYR(lc)......Truncated fraction in delta-m method
   KK(lc)........Eigenvalues in eq. KST(20)
   LAYRU(lu).....Computational layer in which user output level UTAU(lu) is located
   LL(iq,lc).....Constants of integration C-tilde in eqs. KST(42-43) obtained by solving eqs. KST(38-41)
   lyrcut........True, radiation is assumed zero below layer -ncut- because of almost complete absorption
   ncut..........Computational layer number in which absorption optical depth first exceeds abscut
   OPRIM(lc).....Single scattering albedo after delta-m scaling
   pass1.........TRUE on first entry, FALSE thereafter
   PKAG(0:lc)....Integrated Planck function for internal emission at layer boundaries
   PKAGC(lc).....Integrated Planck function for internal emission at layer center
   RR(lc)........Eigenvectors at polar quadrature angles.
   TAUC(0:lc)....Cumulative optical depth (un-delta-m-scaled)
   TAUCPR(0:lc)..Cumulative optical depth (delta-m-scaled if deltam = TRUE, otherwise equal to TAUC)
   tplanck.......Intensity emitted from top boundary
   U0C(iq,lu)....Azimuthally-averaged intensity
   UTAUPR(lu)....Optical depths of user output levels in delta-m coordinates;  equal to UTAU(lu) if no delta-m

   The following are members of the structure twostr_xyz:
   XB_0D(lc).....x-sub-zero-sup-minus in expansion of pseudo-spherical beam source, eq. KST(22)
   XB_0U(lc).....x-sub-zero-sup-plus  in expansion of pseudo-spherical beam source, eq. KST(22)
   XB_1D(lc).....x-sub-one-sup-minus  in expansion of pseudo-spherical beam source, eq. KST(22)
   XB_1U(lc).....x-sub-one-sup-plus   in expansion of pseudo-spherical beam source, eq. KST(22)
   XP_0(lc)......x-sub-zero in expansion of thermal source function; see eq. KST(22) (has no (mu) dependence)
   XP_1(lc)......x-sub-one  in expansion of thermal source function; see eq. KST(22) (has no (mu) dependence)
   YB_0D(lc).....y-sub-zero-sup-minus in eq. KST(23), solution for pseudo-spherical beam source
   YB_0U(lc).....y-sub-zero-sup-plus  in eq. KST(23), solution for pseudo-spherical beam source
   YB_1D(lc).....y-sub-one-sup-minus  in eq. KST(23), solution for pseudo-spherical beam source
   YB_1U(lc).....y-sub-one-sup-plus   in eq. KST(23), solution for pseudo-spherical beam source
   YP_0D(lc).....y-sub-zero-sup-minus in eq. KST(23), solution for thermal source
   YP_0U(lc).....y-sub-zero-sup-plus  in eq. KST(23), solution for thermal source
   YP_1D(lc).....y-sub-one-sup-minus  in eq. KST(23), solution for thermal source
   YP_1U(lc).....y-sub-one-sup-plus   in eq. KST(23), solution for thermal source
   ZB_A(lc)......Alpha coefficient in eq. KST(22) for pseudo-spherical beam source
   ZP_A(lc)......Alpha coefficient in eq. KST(22) for thermal source
*/

DISPATCH_MACRO inline void c_twostr(disort_state  *ds,
              disort_output *out,
              int            deltam,
              double        *gg,
              int           *ierror,
              double         radius,
              emission_func_t emi_func)
{
  int
    lc,ierr;
  int
    lyrcut,iret,ncut,nn;
  int
    *ipvt,
    *layru;
  double
    cmu,bplanck,tplanck;
  double
    *b,*cband,*ch,*chtau,*dtaucpr,*expbea,*flyr,*ggprim,
    *kk,*ll,*oprim,*pkag,*pkagc,*rr,*tauc,*taucpr,*u0c,*utaupr;
  disort_pair
    *fl;
  twostr_xyz
    *ts;
  twostr_diag
    *diag;
  const double
    dither = 100.*DBL_EPSILON;

  /* ipvt and layru were stack VLAs in cdisort.c; VLAs are not valid in
   * C++/device code, so they are pool-allocated instead. */
  ipvt  = (int *)pmalloc(ds->nstr*ds->nlyr*sizeof(int));
  layru = (int *)pmalloc(ds->ntau*sizeof(int));

  /*
   * Allocate zeroed memory
   */
  b       = c_dbl_vector(0,ds->nstr*ds->nlyr-1,"b");
  cband   = c_dbl_vector(0,ds->nstr*ds->nlyr*(9*(ds->nstr/2)-2)-1,"cband");
  ch      = c_dbl_vector(0,ds->nlyr-1,"ch");
  chtau   = c_dbl_vector(0,(2*ds->nlyr+1)-1,"chtau");
  dtaucpr = c_dbl_vector(0,ds->nlyr-1,"dtaucpr");
  expbea  = c_dbl_vector(0,ds->nlyr,"expbea");
  flyr    = c_dbl_vector(0,ds->nlyr-1,"flyr");
  ggprim  = c_dbl_vector(0,ds->nlyr-1,"ggprim");
  kk      = c_dbl_vector(0,ds->nlyr-1,"kk");
  ll      = c_dbl_vector(0,ds->nlyr*ds->nstr-1,"ll");
  oprim   = c_dbl_vector(0,ds->nlyr-1,"oprim");
  pkag    = c_dbl_vector(0,ds->nlyr,"pkag");
  pkagc   = c_dbl_vector(0,ds->nlyr-1,"pkagc");
  rr      = c_dbl_vector(0,ds->nlyr-1,"rr");
  tauc    = c_dbl_vector(0,ds->nlyr,"tauc");
  taucpr  = c_dbl_vector(0,ds->nlyr,"taucpr");
  u0c     = c_dbl_vector(0,ds->ntau*ds->nstr-1,"u0c");
  utaupr  = c_dbl_vector(0,ds->ntau-1,"utaupr");
  /*
   * Using C structures to facilitate cache-aware memory allocation, which tends to
   * reduce cache misses and speed up computer execution.
   */
  fl   = (disort_pair *)pcalloc(ds->ntau,  sizeof(disort_pair)); if (!fl)   c_errmsg("twostr alloc error for fl",  DS_ERROR);
  ts   = (twostr_xyz  *)pcalloc(ds->nlyr,  sizeof(twostr_xyz )); if (!ts)   c_errmsg("twostr alloc error for ts",  DS_ERROR);
  diag = (twostr_diag *)pcalloc(2*ds->nlyr,sizeof(twostr_diag)); if (!diag) c_errmsg("twostr alloc error for diag",DS_ERROR);

  if(ds->flag.prnt[0]) {
    fprintf(stdout,"\n\n\n\n"
            " ************************************************************************************************************************\n"
            "                         Two stream method radiative transfer program, version 1.13\n"
            " ************************************************************************************************************************\n");
  }

  memset(ierror,0,TWOSTR_NERR*sizeof(int));

  /*
   * Calculate cumulative optical depth and dither single-scatter albedo to improve numerical behavior of
   * eigenvalue/vector computation
   */

  for (lc = 1; lc <= ds->nlyr; lc++) {
    if(SSALB(lc) == 1.) {
      SSALB(lc) = 1.-dither;
    }
    TAUC(lc) = TAUC(lc-1)+DTAUC(lc);
  }

  /*
   * Check input dimensions and variables
   */
  c_twostr_check_inputs(ds,gg,ierror,tauc);

  iret = 0;
  for (ierr = 1; ierr <= TWOSTR_NERR; ierr++) {
    if (IERROR(ierr) != 0) {
      iret = 1;
      if (ds->flag.quiet==VERBOSE) {
        fprintf(stderr,"\ntwostr reports fatal error: %d\n",ierr);
      }
    }
  }
  if (iret == 1) {
    goto free_local_memory_and_return;
  }

 /*
  * Perform various setup operations
  */
  c_twostr_set(ds,&bplanck,ch,chtau,&cmu,deltam,dtaucpr,expbea,flyr,gg,ggprim,layru,&lyrcut,
             &ncut,&nn,oprim,pkag,pkagc,radius,tauc,taucpr,&tplanck,utaupr,emi_func);

  /*
   * Print input information
   */
  if (ds->flag.prnt[0]) {
    c_twostr_print_inputs(ds,deltam,flyr,gg,lyrcut,oprim,tauc,taucpr);
  }

  /*
   * Calculate the homogenous and particular solutions
   */
  c_twostr_solns(ds,ch,chtau,cmu,ncut,oprim,pkag,pkagc,taucpr,ggprim,kk,rr,ts);

  /*
   * Solve for constants of integration in homogeneous solution (general boundary conditions)
   */
  c_twostr_solve_bc(ds,ts,bplanck,cband,cmu,expbea,lyrcut,nn,ncut,tplanck,taucpr,kk,rr,ipvt,b,ll,diag);

  /*
   * Compute upward and downward fluxes, mean intensities and flux divergences.
   */
  c_twostr_fluxes(ds,ts,ch,cmu,kk,layru,ll,lyrcut,ncut,oprim,rr,taucpr,utaupr,out,u0c,fl);

  /*
   * Free allocated memory
   */
 free_local_memory_and_return:
  pfree(b),     pfree(cband), pfree(ch),  pfree(chtau),pfree(dtaucpr),pfree(expbea),pfree(flyr),pfree(fl);
  pfree(ggprim),pfree(kk),    pfree(ll),  pfree(oprim),pfree(pkag),   pfree(pkagc), pfree(rr),  pfree(tauc),pfree(taucpr);
  pfree(u0c),   pfree(utaupr),pfree(diag),pfree(ts);
  pfree(ipvt),  pfree(layru);
  
  return;
}

/*============================= end of c_twostr() ========================*/

/*============================= c_twostr_check_inputs() ==================*/

/*
 * Checks the twostr input dimensions and variables
 */

DISPATCH_MACRO inline void c_twostr_check_inputs(disort_state *ds,
                           double       *gg,
                           int          *ierror,
                           double       *tauc)
{
  int
    inperr,lc,lu;
  double
    umumin;

  inperr = FALSE;

  if (ds->nlyr < 1) {
    inperr    = c_write_bad_var(ds->flag.quiet,"nlyr");
    IERROR(1) = 1;
  }

  for (lc = 1; lc <= ds->nlyr; lc++) {
    if (DTAUC(lc) < 0.) {
      inperr     = c_write_bad_var(ds->flag.quiet,"dtauc");
      IERROR(3) += 1;
    }
    if (SSALB(lc) < 0. || SSALB(lc) > 1.) {
      inperr     = c_write_bad_var(ds->flag.quiet,"ssalb");
      IERROR(4) += 1;
    }
    if (ds->flag.planck) {
      if (lc == 1 && TEMPER(0) < 0.) {
        inperr     = c_write_bad_var(ds->flag.quiet,"temper");
        IERROR(5) += 1;
      }
      if (TEMPER(lc) < 0.) {
        inperr     = c_write_bad_var(ds->flag.quiet,"temper");
        IERROR(5) += 1;
      }
    }
    if (GG(lc) < -1. || GG(lc) > 1.) {
      inperr     = c_write_bad_var(ds->flag.quiet,"gg");
      IERROR(6) += 1;
    }
  }

  if(ds->flag.spher==TRUE) {
    for (lc = 1; lc <= ds->nlyr; lc++) {
      if (ds->ZD(lc) > ds->ZD(lc-1)) {
        inperr     = c_write_bad_var(ds->flag.quiet,"zd");
        IERROR(7) += 1;
      }
    }
  }

  if (ds->flag.usrtau) {
    if (ds->ntau < 1) {
      inperr    = c_write_bad_var(ds->flag.quiet,"ntau");
      IERROR(8) = 1;
    }
    for (lu = 1; lu <= ds->ntau; lu++) {
      if (fabs(UTAU(lu)-TAUC(ds->nlyr)) <= 1.e-6*TAUC(ds->nlyr)) { /* relative check copied from c_check_inputs() */
        UTAU(lu)= TAUC(ds->nlyr);
      }
      if (UTAU(lu) < 0. || UTAU(lu) > TAUC(ds->nlyr)) {
        inperr      = c_write_bad_var(ds->flag.quiet,"utau");
        IERROR(10) += 1;
      }
    }
  }

  if (ds->bc.fbeam < 0.) {
    inperr     = c_write_bad_var(ds->flag.quiet,"fbeam");
    IERROR(12) = 1;
  }

  umumin = 0.;
  if(ds->flag.spher==TRUE) {
    umumin = -1.;
  }

  if (ds->bc.fbeam > 0. && (ds->bc.umu0 <= umumin || ds->bc.umu0 > 1.)) {
    inperr     = c_write_bad_var(ds->flag.quiet,"umu0");
    IERROR(13) = 1;
  }
  if (ds->bc.fisot < 0.) {
    inperr     = c_write_bad_var(ds->flag.quiet,"fisot");
    IERROR(14) = 1;
  }
  if (ds->bc.albedo < 0. || ds->bc.albedo > 1.) {
    inperr     = c_write_bad_var(ds->flag.quiet,"albedo");
    IERROR(15) = 1;
  }

  if(ds->flag.planck) {
    if (ds->wvnmlo < 0. || ds->wvnmhi < ds->wvnmlo) {
      inperr     = c_write_bad_var(ds->flag.quiet,"wvnmlo,hi");
      IERROR(16) = 1;
    }
    if (ds->bc.temis < 0. || ds->bc.temis > 1.) {
      inperr     = c_write_bad_var(ds->flag.quiet,"temis");
      IERROR(17) = 1;
    }
    if (ds->bc.btemp < 0.) {
      inperr     = c_write_bad_var(ds->flag.quiet,"btemp");
      IERROR(18) = 1;
    }
    if (ds->bc.ttemp < 0.) {
      inperr     = c_write_bad_var(ds->flag.quiet,"ttemp");
      IERROR(19) = 1;
    }
  }

  if (!ds->flag.usrtau && ds->ntau < ds->nlyr+1) {
    inperr = c_write_too_small_dim(ds->flag.quiet,"ds.ntau",ds->nlyr+1);
    IERROR(22) = 1;
  }

  if (ds->bc.fluor < 0.) {
    inperr     = c_write_bad_var(ds->flag.quiet,"fluor");
    IERROR(23) = 1;
  }

  if (inperr) {
    c_errmsg("twostr_check_inputs--input and/or dimension errors",DS_ERROR);
  }

  for (lc = 1; lc <= ds->nlyr; lc++) {
    if (ds->flag.planck && fabs(TEMPER(lc)-TEMPER(lc-1)) > 50. && ds->flag.quiet==VERBOSE) {
      c_errmsg("twostr_check_inputs--vertical temperature step may be too large for good accuracy",DS_WARNING);
    }
  }

  return;
}

/*============================= end of c_twostr_check_inputs() ===========*/

/*============================= c_twostr_fluxes() ========================*/

/*
 Calculates the radiative fluxes, mean intensity, and flux derivative
 with respect to optical depth from the azimuthally-averaged intensity

 I n p u t     v a r i a b l e s:

   ds         :  'Disort' state variables
   ts         :  twostr_xyz structure variables (xp_0, yb_0d, zb_a...; see cdisort.h)
   ch         :  Chapman factor
   cmu        :  Abscissa for gauss quadrature over angle cosine
   kk         :  Eigenvalues
   layru      :  Layer numbers of user levels -utau-
   ll         :  Constants of integration in eqs. KST(42-43), obtaine by solving eqs. KST(38-41)
   lyrcut     :  Logical flag for truncation of comput. layer
   ncut       :  Number of computational layer where absorption optical depth exceeds -abscut-
   oprim      :  Delta-m scaled single scattering albedo
   rr         :  Eigenvectors at polar quadrature angles
   flag.spher :  TRUE turns on pseudo-spherical effects
   taucpr     :  Cumulative optical depth (delta-m-scaled)
   utaupr     :  Optical depths of user output levels in delta-m coordinates; equal to  -utau- if no delta-m

 O u t p u t     v a r i a b l e s:

   out      :  'Disort' output variables
   u0c      :  Azimuthally averaged intensities at polar quadrature angle cmu

 I n t e r n a l       v a r i a b l e s:

   dirint   :  direct intensity attenuated
   fdntot   :  total downward flux (direct + diffuse)
   fldir    :  fl[].zero, direct-beam flux (delta-m scaled)
   fldn     :  fl[].one, diffuse down-flux (delta-m scaled)
   fnet     :  net flux (total-down - diffuse-up)
   fact     :  EXP( - utaupr / ch ), where ch is the Chapman factor
   plsorc   :  Planck source function (thermal)
 ---------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_twostr_fluxes(disort_state  *ds,
                     twostr_xyz    *ts,
                     double        *ch,
                     double         cmu,
                     double        *kk,
                     int           *layru,
                     double        *ll,
                     int            lyrcut,
                     int            ncut,
                     double        *oprim,
                     double        *rr,
                     double        *taucpr,
                     double        *utaupr,
                     disort_output *out,
                     double        *u0c,
                     disort_pair   *fl)
{
  int
    lu,lyu;
  double
    fdntot,fnet,plsorc,dirint;
  double
    fact1,fact2;

  if (ds->flag.prnt[1]) {
    fprintf(stdout,"\n\n                     <----------------------- Fluxes ----------------------->\n"
                   "   optical  compu    downward    downward    downward       upward                    mean      Planck   d(net flux)\n"
                   "     depth  layer      direct     diffuse       total      diffuse         net   intensity      source   / d(op dep)\n");
  }

  memset(out->rad,0,ds->ntau*sizeof(disort_radiant));

  /*
   * Loop over user levels
   */
  if (ds->flag.planck) {
    for (lu = 1; lu <= ds->ntau; lu++) {
      lyu        = LAYRU(lu);
      fact1      = exp(-ZP_A(lyu)*UTAUPR(lu));
      U0C(1,lu) += fact1*(YP_0D(lyu)+YP_1D(lyu)*UTAUPR(lu));
      U0C(2,lu) += fact1*(YP_0U(lyu)+YP_1U(lyu)*UTAUPR(lu));
    }
  }
  for (lu = 1; lu <= ds->ntau; lu++) {
    lyu = LAYRU(lu);
    if (lyrcut && lyu > ncut) {
      /*
       * No radiation reaches this level
       */
      fdntot = 0.;
      fnet   = 0.;
      plsorc = 0.;
    }
    else {
      if (ds->bc.fbeam > 0.) {
        fact1      = exp(-ZB_A(lyu)*UTAUPR(lu));
        U0C(1,lu) += fact1*(YB_0D(lyu)+YB_1D(lyu)*UTAUPR(lu));
        U0C(2,lu) += fact1*(YB_0U(lyu)+YB_1U(lyu)*UTAUPR(lu));
        if (ds->bc.umu0 > 0. || ds->flag.spher) {
          fact1      = ds->bc.fbeam*exp(-UTAUPR(lu)/CH(lyu));
          dirint     = fact1;
          FLDIR(lu)  = fabs(ds->bc.umu0)*fact1;
          RFLDIR(lu) = fabs(ds->bc.umu0)*ds->bc.fbeam*exp(-UTAU(lu)/CH(lyu));
        }
        else {
          dirint     = 0.;
          FLDIR(lu)  = 0.;
          RFLDIR(lu) = 0.;
        }
      }
      else {
        dirint     = 0.;
        FLDIR(lu)  = 0.;
        RFLDIR(lu) = 0.;
      }
      fact1      = LL(1,lyu)*exp( KK(lyu)*(UTAUPR(lu)-TAUCPR(lyu  )));
      fact2      = LL(2,lyu)*exp(-KK(lyu)*(UTAUPR(lu)-TAUCPR(lyu-1)));
      U0C(1,lu) += fact2+RR(lyu)*fact1;
      U0C(2,lu) += fact1+RR(lyu)*fact2;
      /*
       * Calculate fluxes and mean intensities; downward and upward fluxes from eq. KST(9)
       */
      fact1     = 2.*M_PI*cmu;
      FLDN(lu)  = fact1*U0C(1,lu);
      FLUP(lu)  = fact1*U0C(2,lu);
      fdntot    = FLDN(lu)+FLDIR(lu);
      fnet      = fdntot-FLUP(lu);
      RFLDN(lu) = fdntot-RFLDIR(lu);
      /*
       * Mean intensity from eq. KST(10)
       */
      UAVG(lu) = U0C(1,lu)+U0C(2,lu);
      UAVG(lu) = (2.*M_PI*UAVG(lu)+dirint)/(4.*M_PI);

      /*
       * Flux divergence from eqs. KST(11-12)
       */
      plsorc   = 1./(1.-OPRIM(lyu))*exp(-ZP_A(lyu)*UTAUPR(lu))*(XP_0(lyu)+XP_1(lyu)*UTAUPR(lu));
      DFDT(lu) = (1.-SSALB(lyu))*4.*M_PI*(UAVG(lu)-plsorc);
    }
    if (ds->flag.prnt[1]) {
      fprintf(stdout,"%10.4f%7d%12.3e%12.3e%12.3e%12.3e%12.3e%12.3e%12.3e%14.3e\n",
                     UTAU(lu),lyu,RFLDIR(lu),RFLDN(lu),fdntot,FLUP(lu),fnet,UAVG(lu),plsorc,DFDT(lu));
    }
  }

  return;
}

/*============================= end of c_twostr_fluxes() =================*/

/*============================= c_twostr_solns() =========================*/

/*
    Calculates the homogenous and particular solutions to the
    radiative transfer equation in the two-stream approximation,
    for each layer in the medium.

    I n p u t     v a r i a b l e s:

      ds         : 'Disort' state variables
      ch         : Chapman correction factor
      chtau      :
      cmu        : Abscissa for gauss quadrature over angle cosine
      ncut       : Number of computational layer where absorption optical depth exceeds -abscut-
      oprim      : Delta-m scaled single scattering albedo
      pkag,c     : Planck function in each layer
      flag.spher : spher = true => spherical geometry invoked
      taucpr     : Cumulative optical depth (delta-m-scaled)
      ggprim     :

   O u t p u t     v a r i a b l e s:

      kk         :  Eigenvalues
      rr         :  Eigenvectors at polar quadrature angles
      ts         :  twostr_xyz structure variables (see cdisort.h)
  ----------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_twostr_solns(disort_state *ds,
                    double       *ch,
                    double       *chtau,
                    double        cmu,
                    int           ncut,
                    double       *oprim,
                    double       *pkag,
                    double       *pkagc,
                    double       *taucpr,
                    double       *ggprim,
                    double       *kk,
                    double       *rr,
                    twostr_xyz   *ts)
{
  int
    lc;
  int
    initialized = FALSE;
  double
    big,large,small,little;
  double
    q_1,q_2,qq,q0a,q0,q1a,q2a,q1,q2,
    deltat,denomb,z0p,z0m,arg,sgn,fact3,denomp,
    beta,fact1,fact2;

  if (!initialized) {
    /*
     * The calculation of the particular solutions require some care; small,little,
       big, and large have been set so that no problems should occur in double precision.
     */
    small  = 1.e+30*DBL_MIN;
    little = 1.e+20*DBL_MIN;
    big    = sqrt(DBL_MAX)/1.e+10;
    large  = log(DBL_MAX)-20.;

    initialized = TRUE;
  }

  /*----------------  Begin loop on computational layers  ---------------------*/

  for (lc = 1; lc <= ncut; lc++) {
    /*
     * Calculate eigenvalues -kk- and eigenvector -rr-, eqs. KST(20-21)
     */
    beta   = 0.5*(1.-3.*GGPRIM(lc)*cmu*cmu);
    fact1  = 1.-OPRIM(lc);
    fact2  = 1.-OPRIM(lc)+2.*OPRIM(lc)*beta;
    KK(lc) = (1./cmu)*sqrt(fact1*fact2);
    RR(lc) = (sqrt(fact2)-sqrt(fact1))/(sqrt(fact2)+sqrt(fact1));

    if (ds->bc.fbeam > 0.) {
      /*
       * Set coefficients in KST(22) for beam source
       */
      q_1 = ds->bc.fbeam/(4.*M_PI)*OPRIM(lc)*(1.-3.*GGPRIM(lc)*cmu*ds->bc.umu0);
      q_2 = ds->bc.fbeam/(4.*M_PI)*OPRIM(lc)*(1.+3.*GGPRIM(lc)*cmu*ds->bc.umu0);

      if (ds->bc.umu0 >= 0.) {
        qq = q_2;
      }
      else {
        qq = q_1;
      }

      if (ds->flag.spher) {
        q0a = exp(-CHTAU(lc-1));
        q0  = q0a*qq;
        if (q0 <= small) {
          q1a = 0.;
          q2a = 0.;
        }
        else {
          q1a = exp(-CHTAU(lc-1  ));
          q2a = exp(-CHTAU(lc));
        }
      }
      else {
        q0a = exp(-TAUCPR(lc-1)/ds->bc.umu0);
        q0  = q0a*qq;
        if (q0 <= small) {
          q1a = 0.;
          q2a = 0.;
        }
        else {
          q1a = exp(-(TAUCPR(lc-1)+TAUCPR(lc))/(2.*ds->bc.umu0));
          q2a = exp(-TAUCPR(lc)/ds->bc.umu0);
        }
      }
      q1 = q1a*qq;
      q2 = q2a*qq;

      /*
       * Calculate alpha coefficient
       */
      deltat     = TAUCPR(lc)-TAUCPR(lc-1);
      ZB_A(lc)   = 1./CH(lc);
      if (fabs(ZB_A(lc)*TAUCPR(lc-1)) > large || fabs(ZB_A(lc)*TAUCPR(lc)) > large) {
        ZB_A(lc) = 0.;
      }

      /*
       * Dither alpha if it is close to an eigenvalue
       */
      denomb = fact1*fact2-SQR(ZB_A(lc)*cmu);
      if (denomb < 1.e-03) {
        ZB_A(lc) = 1.02*ZB_A(lc);
      }
      q0 = q0a*q_1;
      q2 = q2a*q_1;

      /*
       * Set constants in eq. KST(22)
       */
      if (deltat < 1.e-07) {
        XB_1D(lc) = 0.;
      }
      else {
        XB_1D(lc) = 1./deltat*(q2*exp(ZB_A(lc)*TAUCPR(lc))-q0*exp(ZB_A(lc)*TAUCPR(lc-1)));
      }
      XB_0D(lc) = q0*exp(ZB_A(lc)*TAUCPR(lc-1))-XB_1D(lc)*TAUCPR(lc-1);
      q0        = q0a*q_2;
      q2        = q2a*q_2;

      if (deltat < 1.e-07) {
        XB_1U(lc) = 0.;
      }
      else {
        XB_1U(lc) = 1./deltat*(q2*exp(ZB_A(lc)*TAUCPR(lc))-q0*exp(ZB_A(lc)*TAUCPR(lc-1)));
      }
      XB_0U(lc) = q0*exp(ZB_A(lc)*TAUCPR(lc-1))-XB_1U(lc)*TAUCPR(lc-1);

      /*
       * Calculate particular solutions for incident beam source in pseudo-spherical geometry, eqs. KST(24-25)
       */
      denomb    = fact1*fact2-SQR(ZB_A(lc)*cmu);
      YB_1D(lc) = (OPRIM(lc)*beta*XB_1D(lc)+(1.-OPRIM(lc)*(1.-beta)+ZB_A(lc)*cmu)*XB_1U(lc))/denomb;
      YB_1U(lc) = (OPRIM(lc)*beta*XB_1U(lc)+(1.-OPRIM(lc)*(1.-beta)-ZB_A(lc)*cmu)*XB_1D(lc))/denomb;
      z0p       = XB_0U(lc)-cmu*YB_1D(lc);
      z0m       = XB_0D(lc)+cmu*YB_1U(lc);
      YB_0D(lc) = (OPRIM(lc)*beta*z0m+(1.-OPRIM(lc)*(1.-beta)+ZB_A(lc)*cmu)*z0p)/denomb;
      YB_0U(lc) = (OPRIM(lc)*beta*z0p+(1.-OPRIM(lc)*(1.-beta)-ZB_A(lc)*cmu)*z0m)/denomb;
    }

    if(ds->flag.planck) {
      /*
       * Set coefficients in KST(22) for thermal source
       * Calculate alpha coefficient
       */
      q0     = (1.-OPRIM(lc))*PKAG(lc-1);
      q1     = (1.-OPRIM(lc))*PKAGC(lc);
      q2     = (1.-OPRIM(lc))*PKAG(lc);
      deltat = TAUCPR(lc)-TAUCPR(lc-1);

      if ((q2 < q0*1.e-02 || q2 <= little) && q1 > little && q0 > little) {
        /*
         * Case 1: source small at bottom layer; alpha eq. KS(50)
         */
        ZP_A(lc) = MIN(2./deltat*log(q0/q1),big);
        if (ZP_A(lc)*TAUCPR(lc-1) >= log(big)) {
          XP_0(lc) = big;
        }
        else {
          XP_0(lc) = q0;
        }
        XP_1(lc) = 0.;
      }
      else if ((q2 <= q1*1.e-02 || q2 <= little) && (q1 <= q0*1.e-02 || q1 <= little) && q0 > little) {
        /*
         * Case 2: Source small at center and bottom of layer
         */
        ZP_A(lc) = big/TAUCPR(ncut);
        XP_0(lc) = q0;
        XP_1(lc) = 0.;
      }
      else if (q2 <= little && q1 <= little && q0 <= little) {
        /*
         * Case 3: All sources zero
         */
        ZP_A(lc) = 0.;
        XP_0(lc) = 0.;
        XP_1(lc) = 0.;
      }
      else if ( ( fabs((q2-q0)/q2) < 1.e-04 && fabs((q2-q1)/q2) < 1.e-04 ) || deltat < 1.e-04) {
        /*
         * Case 4: Sources same at center, bottom and top of layer or layer optically very thin
         */
        ZP_A(lc) = 0.;
        XP_0(lc) = q0;
        XP_1(lc) = 0.;
      }
      else {
        /*
         *  Case 5: Normal case
         */
        arg = MAX(SQR(q1/q2)-q0/q2,0.);
        /*
         * alpha eq. (44). For source that has its maximum at the top of the layer, use negative solution
         */
        sgn = 1.;
        if (PKAG(lc-1) > PKAG(lc)) {
         sgn = -1.;
        }
        fact3 = log(q1/q2+sgn*sqrt(arg));

        /* Be careful with log of numbers close to one */
        if (fabs(fact3) <= 0.005) {
          /* numbers close to one */
          q1    = 0.99*q1; 
          fact3 = log(q1/q2+sgn*sqrt(arg));
        }

        ZP_A(lc) = 2./deltat*fact3;
        if (fabs(ZP_A(lc)*TAUCPR(lc)) > log(DBL_MAX)-log(q0*100.)) {
          ZP_A(lc) = 0.;
        }

        /*
         * Dither alpha if it is close to an eigenvalue
         */
        denomp = fact1*fact2-SQR(ZP_A(lc)*cmu);
        if (denomp < 1.e-03) {
          ZP_A(lc) *= 1.01;
        }

        /*
         * Set constants in eqs. KST(22)
         */
        if(deltat < 1.e-07) {
          XP_1(lc) = 0.;
        }
        else {
          XP_1(lc) = 1./deltat*(q2*exp(ZP_A(lc)*TAUCPR(lc))-q0*exp(ZP_A(lc)*TAUCPR(lc-1)));
        }
        XP_0(lc) = q0*exp(ZP_A(lc)*TAUCPR(lc-1))-XP_1(lc)*TAUCPR(lc-1);
      }

      /*
       * Calculate particular solutions eqs. KST(24-25) for internal thermal so
       */
      denomp    = fact1*fact2-SQR(ZP_A(lc)*cmu);
      YP_1D(lc) = (OPRIM(lc)*beta*XP_1(lc)+(1.-OPRIM(lc)*(1.-beta)+ZP_A(lc)*cmu)*XP_1(lc))/denomp;
      YP_1U(lc) = (OPRIM(lc)*beta*XP_1(lc)+(1.-OPRIM(lc)*(1.-beta)-ZP_A(lc)*cmu)*XP_1(lc))/denomp;
      z0p       = XP_0(lc)-cmu*YP_1D(lc);
      z0m       = XP_0(lc)+cmu*YP_1U(lc);
      YP_0D(lc) = (OPRIM(lc)*beta*z0m+(1.-OPRIM(lc)*(1.-beta)+ZP_A(lc)*cmu)*z0p)/denomp;
      YP_0U(lc) = (OPRIM(lc)*beta*z0p+(1.-OPRIM(lc)*(1.-beta)-ZP_A(lc)*cmu)*z0m)/denomp;
    }
  }

  return;
}

/*============================= end of c_twostr_solns() ==================*/

/*============================= c_twostr_print_inputs() ==================*/

/*
 * Print values of twostream input variables
 */
DISPATCH_MACRO inline void c_twostr_print_inputs(disort_state *ds,
                           int           deltam,
                           double       *flyr,
                           double       *gg,
                           int           lyrcut,
                           double       *oprim,
                           double       *tauc,
                           double       *taucpr)
{
  int
    lu,lc;

  fprintf(stdout,"\n\n"
                 " ****************************************************************************************************\n"
                 " %s\n"
                 " ****************************************************************************************************\n",
                 ds->header);

  fprintf(stdout,"\n No. streams = %4d     No. computational layers =%4d\n",ds->nstr,ds->nlyr);
  fprintf(stdout,"%4d User optical depths :",ds->ntau);
  for (lu = 1; lu <= ds->ntau; lu++) {
    fprintf(stdout,"%10.4f",UTAU(lu));
    if (lu%10 == 0) {
      fprintf(stdout,"\n                          ");
    }
  }
  fprintf(stdout,"\n");

  if (ds->flag.spher) {
    fprintf(stdout," Pseudo-spherical geometry invoked\n");
  }

  if(!ds->flag.planck) {
    fprintf(stdout," No thermal emission\n");
  }

  fprintf(stdout,"    Incident beam with intensity =%11.3e and polar angle cosine = %8.5f\n"
                 "    plus isotropic incident intensity =%11.3e\n",
                 ds->bc.fbeam,ds->bc.umu0,ds->bc.fisot);

  fprintf(stdout,"    Bottom albedo (lambertian) =%8.4f\n",ds->bc.albedo);

  if(ds->flag.planck) {
    fprintf(stdout,"    Thermal emission in wavenumber interval :%14.4f%14.4f\n"
                   "    bottom temperature =%10.2f     top temperature =%10.2f    top emissivity =%8.4f\n",
                   ds->wvnmlo,ds->wvnmhi,ds->bc.btemp,ds->bc.ttemp,ds->bc.temis);
  }

  if(deltam) {
    fprintf(stdout," Uses delta-m method\n");
  }
  else {
    fprintf(stdout," Does not use delta-m method\n");
  }

  if(lyrcut) {
    fprintf(stdout," Sets radiation = 0 below absorption optical depth 10\n");
  }

  if(ds->flag.planck) {
    fprintf(stdout,"\n                                     <------------- delta-m --------------->"
                   "\n                   total    single                           total    single"
                   "\n       optical   optical   scatter   truncated   optical   optical   scatter    asymm"
                   "\n         depth     depth    albedo    fraction     depth     depth    albedo   factor   temperature\n");
  }
  else {
    fprintf(stdout,"\n                                     <------------- delta-m --------------->"
                   "\n                   total    single                           total    single"
                   "\n       optical   optical   scatter   truncated   optical   optical   scatter    asymm"
                   "\n         depth     depth    albedo    fraction     depth     depth    albedo   factor\n");
  }

  for (lc = 1; lc <= ds->nlyr; lc++) {
    if (ds->flag.planck) {
      fprintf(stdout,"%4d%10.4f%10.4f%10.5f%12.5f%10.4f%10.4f%10.5f%9.4f%14.3f\n",
                     lc,DTAUC(lc),TAUC(lc),SSALB(lc),FLYR(lc),TAUCPR(lc)-TAUCPR(lc-1),TAUCPR(lc),OPRIM(lc),GG(lc),TEMPER(lc-1));
    }
    else {
      fprintf(stdout,"%4d%10.4f%10.4f%10.5f%12.5f%10.4f%10.4f%10.5f%9.4f\n",
                     lc,DTAUC(lc),TAUC(lc),SSALB(lc),FLYR(lc),TAUCPR(lc)-TAUCPR(lc-1),TAUCPR(lc),OPRIM(lc),GG(lc));
    }
  }

  if(ds->flag.planck) {
    fprintf(stdout,"                                                                                     %14.3f\n",TEMPER(ds->nlyr));
  }

  return;
}

/*============================= end of c_twostr_print_inputs() ===========*/

/*============================= c_twostr_set() ===========================*/

/*
 Perform miscellaneous setting-up operations

 Routines called: c_errmsg

 Input :  ds         'Disort' input variables

 Output:  ntau,utau  If ds->flag.usrtau = FALSE
          bplanck    Intensity emitted from bottom boundary
          ch         The Chapman factor
          cmu        Computational polar angle
          expbea     Transmission of direct beam
          flyr       Truncated fraction in delta-m method
          layru      Computational layer in which utau falls
          lyrcut     Flag as to whether radiation will be zeroed below layer ncut
          ncut       Computational layer where absorption optical depth first exceeds abscut
          nn         nstr/2 = 1
          nstr       No.of streams (=2)
          oprim      Delta-m-scaled single-scatter albedo
          pkag,c     Planck function in each layer
          taucpr     Delta-m-scaled optical depth
          tplanck    Intensity emitted from top boundary
          utaupr     Delta-m-scaled version of utau

 Internal Variables
          abscut     Absorption optical depth, medium is cut off below this depth
          tempc      Temperature at center of layer, assumed to be average of
                     layer boundary temperatures
  ---------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_twostr_set(disort_state *ds,
                  double       *bplanck,
                  double       *ch,
                  double       *chtau,
                  double       *cmu,
                  int           deltam,
                  double       *dtaucpr,
                  double       *expbea,
                  double       *flyr,
                  double       *gg,
                  double       *ggprim,
                  int          *layru,
                  int          *lyrcut,
                  int          *ncut,
                  int          *nn,
                  double       *oprim,
                  double       *pkag,
                  double       *pkagc,
                  double        radius,
                  double       *tauc,
                  double       *taucpr,
                  double       *tplanck,
                  double       *utaupr,
                  emission_func_t emi_func)
{
  int
    firstpass = TRUE;
  int
    lc,lu,lev;
  double
    zenang,abstau,chtau_tmp,f,tempc,taup,
    abscut = 10.;

  if (firstpass) {
    firstpass = FALSE;
    ds->nstr  = 2;
    *nn       = ds->nstr/2;
  }

  if (!ds->flag.usrtau) {
    /*
     * Set output levels at computational layer boundaries
     */
    ds->ntau = ds->nlyr+1;
    for (lc = 0; lc <= ds->ntau-1; lc++) {
      UTAU(lc+1) = TAUC(lc);
    }
  }
  /*
   * Apply delta-m scaling and move description of computational layers to local variables
   */

  /*
   * NOTE: If not using calloc() to dynamically allocate memory, then need to zero-out
   *       taucpr, expbea, flyr, oprim here.
   */

  abstau = 0.;
  for (lc = 1; lc <= ds->nlyr; lc++) {
    if (abstau < abscut) {
      *ncut = lc;
    }
    abstau += (1.-SSALB(lc))*DTAUC(lc);
    if (!deltam) {
      OPRIM(lc)   = SSALB(lc);
      TAUCPR(lc)  = TAUC(lc);
      f           = 0.;
      GGPRIM(lc)  = GG(lc);
      DTAUCPR(lc) = DTAUC(lc);
    }
    else {
     /*
      * Do delta-m transformation eqs. WW(20a,20b,14)
      */
      f           = SQR(GG(lc));
      TAUCPR(lc)  = TAUCPR(lc-1)+(1.-f*SSALB(lc))*DTAUC(lc);
      OPRIM(lc)   = SSALB(lc)*(1.-f)/(1.-f*SSALB(lc));
      GGPRIM(lc)  = (GG(lc)-f)/(1.-f);
      DTAUCPR(lc) = TAUCPR(lc)-TAUCPR(lc-1);
    }
    FLYR(lc) = f;
  }
  /*
   * If no thermal emission, cut off medium below absorption optical
   * depth = abscut (note that delta-m transformation leaves absorption
   * optical depth invariant). Not worth the trouble for one-layer problems, though.
   */
  *lyrcut = FALSE;
  if (abstau >= abscut && !ds->flag.planck && ds->nlyr > 1) {
    *lyrcut = TRUE;
  }
  if (!*lyrcut) {
    *ncut = ds->nlyr;
  }
  /*
   * Calculate Chapman function if spherical geometry, set expbea and ch for beam source.
   */
  if (ds->bc.fbeam > 0.) {
    CHTAU(0) = 0.;
    EXPBEA(0) = 1.;
    zenang    = acos(ds->bc.umu0)/DEG;
    
    if(ds->flag.spher == TRUE && ds->bc.umu0 < 0.) {
      EXPBEA(0) = exp(-c_chapman(1,0.,tauc,ds->nlyr,ds->zd,ds->dtauc,zenang,radius));
    }
    if (ds->flag.spher == TRUE) {
      for (lc = 1; lc <= *ncut; lc++) {
        taup        = TAUCPR(lc-1)+DTAUCPR(lc)/2.;
        CHTAU(lc  ) = c_chapman(lc, 0.0,      taucpr,ds->nlyr,ds->zd,dtaucpr,zenang,radius);
        chtau_tmp   = c_chapman(lc, 0.5,taucpr,ds->nlyr,ds->zd,dtaucpr,zenang,radius);
        CH(lc)      = taup/chtau_tmp;
        EXPBEA(lc)  = exp(-CHTAU(lc));
      }
    }
    else {
      for (lc = 1; lc <= *ncut; lc++) {
        CH(lc)     = ds->bc.umu0;
        EXPBEA(lc) = exp(-TAUCPR(lc)/ds->bc.umu0);
      }
    }
  }
  /*
   * Set arrays defining location of user output levels within delta-m-scaled computational mesh
   */
  for (lu = 1; lu <= ds->ntau; lu++) {
    for (lc = 1; lc <= ds->nlyr-1; lc++) {
      if (UTAU(lu) >= TAUC(lc-1) && UTAU(lu) <= TAUC(lc)) {
        break;
      }
    }
    UTAUPR(lu) = UTAU(lu);
    if (deltam) {
      UTAUPR(lu) = TAUCPR(lc-1)+(1.-SSALB(lc)*FLYR(lc))*(UTAU(lu)-TAUC(lc-1));
    }
    LAYRU(lu) = lc;
  }

  /*
   * Set computational polar angle cosine for double gaussian
   * quadrature; cmu = 0.5, or  single gaussian quadrature; cmu = 1./sqrt(3
   * See KST for discussion of which is better for your specific applicatio
   */
  if(ds->flag.planck && ds->bc.fbeam == 0.) {
    *cmu = 0.5;
  }
  else {
    *cmu = sqrt(1./3.);
  }
  /*
   * Calculate planck functions
   */
  if (!ds->flag.planck) {
    *bplanck = 0.;
    *tplanck = 0.;
    /*
     * NOTE: If not using calloc() for dynamic memory allocation, need to zero-out
     *       pkag and pkagc here.
     */
  }
  else {
    *tplanck = emi_func(ds->wvnmlo,ds->wvnmhi,ds->bc.ttemp)*ds->bc.temis;
    *bplanck = emi_func(ds->wvnmlo,ds->wvnmhi,ds->bc.btemp);
    for (lev = 0; lev <= ds->nlyr; lev++) {
      PKAG(lev) = emi_func(ds->wvnmlo,ds->wvnmhi,TEMPER(lev));
    }
    for (lc = 1; lc <=ds->nlyr; lc++) {
      tempc     = .5*(TEMPER(lc-1)+TEMPER(lc));
      PKAGC(lc) = emi_func(ds->wvnmlo,ds->wvnmhi,tempc);
    }
  }

  return;
}

/*============================= end of c_twostr_set() ====================*/

/*============================= c_twostr_solve_bc() ======================*/

/*
 Construct right-hand side vector -b- for general boundary conditions
 and solve system of equations obtained from the boundary conditions
 and the continuity-of-intensity-at-layer-interface equations.

 Routines called: c_sgbfa, c_sgbsl

 I n p u t      v a r i a b l e s:

       ds       : 'Disort' state variables
       ts       :  twostr_xyz structure variables (see cdisort.h)
       bplanck  :  Bottom boundary thermal emission
       cband    :  Left-hand side matrix of linear system eqs. KST(38-41)
                   in banded form required by linpack solution routines
       cmu      :  Abscissa for gauss quadrature over angle cosine
       expbea   :  Transmission of incident beam, EXP(-taucpr/ch)
       lyrcut   :  Logical flag for truncation of comput. layer
       ncol     :  Counts of columns in -cband-
       nn       :  Order of double-gauss quadrature (nstr/2)
       ncut     :  Total number of computational layers considered
       tplanck  :  Top boundary thermal emission
       taucpr   :  Cumulative optical depth (delta-m-scaled)
       kk       :
       rr       :
       ipvt     :

 O u t p u t     v a r i a b l e s:

       b        :  Right-hand side vector of eqs. KST(38-41) going into
                   sgbsl; returns as solution vector of eqs. KST(38-41)
                   constants of integration
       ll       :  Permanent storage for -b-, but re-ordered

 I n t e r n a l    v a r i a b l e s:

       diag     : diag[].super, diag[].on, diag[].sub

 ---------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_twostr_solve_bc(disort_state *ds,
                       twostr_xyz   *ts,
                       double        bplanck,
                       double       *cband,
                       double        cmu,
                       double       *expbea,
                       int           lyrcut,
                       int           nn,
                       int           ncut,
                       double        tplanck,
                       double       *taucpr,
                       double       *kk,
                       double       *rr,
                       int          *ipvt,
                       double       *b,
                       double       *ll,
                       twostr_diag  *diag)
{
  int
    info;
  int
    irow,lc,nloop,nrow,job;
  double
    wk0,wk1,wk,rpp1_m,rp_m,rpp1_p,rp_p,sum,refflx;
  double
    fact1,fact2,fact3,fact4;

  /*
   * First top row, top boundary condition
   */
  irow = 1;
  lc   = 1;
  /*
   * SUBD(irow) is undefined
   */
  DIAG(irow)   = RR(lc)*exp(-KK(lc)*TAUCPR(lc));
  SUPERD(irow) = 1.;
  /*
   * next from layer no. 2 to nlyr-1
   */
  nloop = ncut-1;
  for (lc = 1; lc <= nloop; lc++) {
    irow++;
    wk0          = exp(-KK(lc  )*(TAUCPR(lc  )-TAUCPR(lc-1)));
    wk1          = exp(-KK(lc+1)*(TAUCPR(lc+1)-TAUCPR(lc  )));
    SUBD(irow)   = 1.-RR(lc)*RR(lc+1);
    DIAG(irow)   = (RR(lc)-RR(lc+1))*wk0;
    SUPERD(irow) = -(1.-SQR(RR(lc+1)))*wk1;
    irow++;
    SUBD(irow)   = (1.-SQR(RR(lc)))*wk0;
    DIAG(irow)   = (RR(lc)-RR(lc+1))*wk1;
    SUPERD(irow) = -(1.-RR(lc+1)*RR(lc));
  }
  /*
   * bottom layer
   */
  irow++;
  lc = ncut;
  /*
   * SUPERD(irow) = undefined
   */
  wk = exp(-KK(lc)*(TAUCPR(lc)-TAUCPR(lc-1)));
  if (lyrcut) {
    SUBD(irow) = 1.;
    DIAG(irow) = RR(lc)*wk;
  }
  else {
    SUBD(irow) = 1.-2.*ds->bc.albedo*cmu*RR(lc);
    DIAG(irow) = (RR(lc)-2.*ds->bc.albedo*cmu)*wk;
  }

  /*
   * NOTE: If not allocating memory with calloc(), need to zero out b here.
   */

  /*
   * Construct -b-, for parallel beam + bottom reflection + thermal emission at top and/or bottom
   * 
   * Top boundary, right-hand-side of eq. KST(28)
   */
  lc   = 1;
  irow = 1;
  B(irow) = -YB_0D(lc)-YP_0D(lc)+ds->bc.fisot+tplanck;
  /*
   * Continuity condition for layer interfaces, right-hand-side of eq. KST(29)
   */
  for (lc = 1; lc <= nloop; lc++) {
    fact1     = exp(-ZB_A(lc+1)*TAUCPR(lc));
    fact2     = exp(-ZP_A(lc+1)*TAUCPR(lc));
    fact3     = exp(-ZB_A(lc  )*TAUCPR(lc));
    fact4     = exp(-ZP_A(lc  )*TAUCPR(lc));
    rpp1_m    = fact1*(YB_0D(lc+1)+YB_1D(lc+1)*TAUCPR(lc))+fact2*(YP_0D(lc+1)+YP_1D(lc+1)*TAUCPR(lc));
    rp_m      = fact3*(YB_0D(lc  )+YB_1D(lc  )*TAUCPR(lc))+fact4*(YP_0D(lc  )+YP_1D(lc  )*TAUCPR(lc));
    rpp1_p    = fact1*(YB_0U(lc+1)+YB_1U(lc+1)*TAUCPR(lc))+fact2*(YP_0U(lc+1)+YP_1U(lc+1)*TAUCPR(lc));
    rp_p      = fact3*(YB_0U(lc  )+YB_1U(lc  )*TAUCPR(lc))+fact4*(YP_0U(lc  )+YP_1U(lc  )*TAUCPR(lc));
    B(++irow) = rpp1_p-rp_p-RR(lc+1)*(rpp1_m-rp_m);
    B(++irow) = rpp1_m-rp_m-RR(lc  )*(rpp1_p-rp_p);
  }
  /*
   * Bottom boundary
   */
  lc = ncut;
  if (lyrcut) {
    /*
     * Right-hand-side of eq. KST(30)
     */
    B(++irow) = -exp(-ZB_A(ncut)*TAUCPR(ncut))*(YB_0U(ncut)+YB_1U(ncut)*TAUCPR(ncut))
                -exp(-ZP_A(ncut)*TAUCPR(ncut))*(YP_0U(ncut)+YP_1U(ncut)*TAUCPR(ncut));
  }
  else {
    sum = cmu*ds->bc.albedo*(exp(-ZB_A(ncut)*TAUCPR(ncut))*(YB_0D(ncut)+YB_1D(ncut)*TAUCPR(ncut))
                            +exp(-ZP_A(ncut)*TAUCPR(ncut))*(YP_0D(ncut)+YP_1D(ncut)*TAUCPR(ncut)));
   if (ds->bc.umu0 <= 0.) {
     refflx = 0.;
   }
   else {
     refflx = 1.;
   }
   B(++irow) = 2.*sum+ds->bc.albedo*ds->bc.umu0*ds->bc.fbeam/M_PI*refflx*EXPBEA(ncut)+(1.-ds->bc.albedo)*bplanck
               -exp(-ZB_A(ncut)*TAUCPR(ncut))*(YB_0U(ncut)+YB_1U(ncut)*TAUCPR(ncut))
               -exp(-ZP_A(ncut)*TAUCPR(ncut))*(YP_0U(ncut)+YP_1U(ncut)*TAUCPR(ncut));

 }
 /*
  * solve for constants of integration by inverting matrix KST(38-41)
  */
  nrow = irow;

  /*
   * NOTE: If not allocating memory with calloc(), need to zero out cband here.
   */

  for (irow = 1; irow <= nrow; irow++) {
    CBAND(1,irow) = 0.;
    CBAND(3,irow) = DIAG(irow);
  }
  for (irow = 1; irow <= nrow-1; irow++) {
    CBAND(2,irow+1) = SUPERD(irow);
  }
  for (irow = 2; irow <= nrow; irow++) {
    CBAND(4,irow-1) = SUBD(irow);
  }

  c_sgbfa(cband,(9*(ds->nstr/2)-2),nrow,1,1,ipvt,&info);
  job = 0;
  c_sgbsl(cband,(9*(ds->nstr/2)-2),nrow,1,1,ipvt,b,job);

  /*
   * unpack
   */
  irow = 0;
  for (lc = 1; lc <= ncut; lc++) {
    /* downward direction */
    LL(1,lc) = B(++irow);

    /* upward direction */
    LL(2,lc) = B(++irow);
  }

  return;
}

/*============================= end of c_twostr_solve_bc() ===============*/

/* Restore the disort-style KK macro from cdisort.h for any header
 * included after this one. */
#undef  KK
#define KK(iq,lc)        kk[iq-1+(lc-1)*ds->nstr]

