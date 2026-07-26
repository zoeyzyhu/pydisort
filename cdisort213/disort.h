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

/*============================= c_disort_work_size() ====================*/

/*
 * Upper bound on the heap bytes one c_disort run needs, used to size the
 * per-thread device pool for the CUDA path.  Mirrors, with the same
 * expressions, every allocation such a run makes:
 *   - c_disort_state_alloc() / c_disort_out_alloc()   (see alloc.h)
 *   - c_disort()'s own work arrays (the general-case list below; the
 *     ibcnd == SPECIAL_BC list is a subset of it), including ipvt/layru
 *     which were stack VLAs in cdisort.c
 *   - transients of c_new_intensity_correction() / prep_double_scat_integr()
 * plus per-allocation alignment padding and a slack factor.  pmem's device
 * pool is a bump allocator (see pmem.h), so there's no header/footer/
 * fragmentation overhead to account for here -- only 8-byte alignment
 * padding per allocation.
 * Keep this in sync when touching those allocation lists.
 */
DISPATCH_MACRO inline size_t c_disort_work_size(disort_state const *ds)
{
  const size_t nlyr  = (size_t)ds->nlyr;
  const size_t nstr  = (size_t)ds->nstr;
  const size_t nphi  = (size_t)(ds->nphi > 0 ? ds->nphi : 1);
  const size_t nphase = (size_t)(ds->nphase > 0 ? ds->nphase : 1);
  const size_t nmom_nstr =
      (size_t)(ds->nmom > ds->nstr ? ds->nmom : ds->nstr);

  /* dimension fixups applied by c_disort_state_alloc / c_disort */
  const size_t ntau = ds->flag.usrtau ? (size_t)ds->ntau : nlyr + 1;
  size_t numu = (!ds->flag.usrang || ds->flag.onlyfl) ? nstr : (size_t)ds->numu;
  if (numu < (size_t)ds->numu) numu = (size_t)ds->numu;
  if (ds->flag.ibcnd == SPECIAL_BC) numu *= 2;

  size_t n_dbl = 0;   /* double elements */
  size_t n_int = 0;   /* int elements */
  size_t n_pair = 0;  /* disort_pair elements */
  size_t n_trip = 0;  /* disort_triplet elements */
  size_t n_rad = 0;   /* disort_radiant elements */
  size_t nblocks = 0; /* individual allocations */

  /* --- c_disort_state_alloc --- */
  n_dbl += (nlyr + 1) + (nlyr + 1);            /* dtauc, ssalb  */
  n_dbl += (nmom_nstr + 1) * nlyr;             /* pmom          */
  if (ds->flag.planck) n_dbl += nlyr + 1;      /* temper        */
  if (ds->flag.general_source)
    n_dbl += nstr * nlyr * nstr + 1 + nstr * nlyr * numu + 1; /* gensrc(u) */
  n_dbl += ntau;                               /* utau          */
  n_dbl += nlyr + 2;                           /* zd            */
  n_dbl += 2 * numu + 1;                       /* umu           */
  n_dbl += nphi;                               /* phi           */
  if (!ds->flag.old_intensity_correction)
    n_dbl += nphase + nlyr * nphase;           /* mu_phase, phase */
  nblocks += 12;

  /* --- c_disort_out_alloc --- */
  n_rad += ntau;                               /* rad           */
  if (!ds->flag.onlyfl) {
    n_dbl += nphi * numu * ntau + 1;           /* uu            */
    n_dbl += ntau * numu + 1;                  /* u0u           */
  }
  if (ds->flag.output_uum) n_dbl += nstr * numu * ntau + 1; /* uum */
  if (ds->flag.ibcnd == SPECIAL_BC) n_dbl += 2 * (numu + 1); /* albmed, trnmed */
  nblocks += ds->flag.onlyfl ? 4 : 6;

  /* --- c_disort work arrays (general case) --- */
  n_int += nstr * nlyr + ntau;                 /* ipvt, layru   */
  n_dbl += nlyr + 1;                           /* tauc          */
  n_dbl += nstr * nstr;                        /* array         */
  n_dbl += nstr * nlyr;                        /* b             */
  n_dbl += ((nstr / 2) + 1) * (nstr / 2);      /* bdr           */
  n_dbl += nstr / 2;                           /* bem           */
  n_dbl += nstr * nlyr * (9 * (nstr / 2) - 2); /* cband         */
  n_dbl += nstr * nstr;                        /* cc            */
  n_dbl += nlyr;                               /* ch            */
  n_dbl += 2 * nlyr + 1;                       /* chtau         */
  n_dbl += nstr + nstr;                        /* cmu, cwt      */
  n_dbl += nlyr;                               /* dtaucpr       */
  n_dbl += numu;                               /* emu           */
  n_dbl += nstr / 2;                           /* eval          */
  n_dbl += nstr * nstr;                        /* evecc         */
  n_dbl += (nlyr + 1) + (nlyr + 1);            /* expbea, flyr  */
  n_dbl += nlyr * nstr * nstr;                 /* gc            */
  n_dbl += nlyr * (nstr + 1) + 1;              /* gl            */
  n_dbl += nlyr * nstr * numu;                 /* gu            */
  n_dbl += nlyr * nstr + nlyr * nstr;          /* kk, ll        */
  n_dbl += nlyr;                               /* oprim         */
  n_dbl += nlyr + nlyr + nlyr;                 /* phasa, phast, phasm */
  n_dbl += nphi;                               /* phirad        */
  n_dbl += nlyr + 1;                           /* pkag          */
  n_dbl += ((nstr / 2) + 1) * numu;            /* rmu           */
  n_dbl += nlyr + 1;                           /* taucpr        */
  n_dbl += ntau * nstr;                        /* u0c           */
  n_dbl += ntau;                               /* utaupr        */
  n_dbl += ntau * numu;                        /* uum           */
  n_dbl += nstr;                               /* wk            */
  n_dbl += nlyr + 1;                           /* xba           */
  n_dbl += nstr + 1;                           /* ylm0          */
  n_dbl += nstr * (nstr + 1);                  /* ylmc          */
  n_dbl += numu * (nstr + 1);                  /* ylmu          */
  n_dbl += nstr * nlyr;                        /* z             */
  n_dbl += nlyr * numu;                        /* zbeam         */
  n_dbl += nlyr + 1;                           /* zbeama        */
  n_dbl += nstr + nstr;                        /* zj, zjg       */
  n_dbl += numu + 1;                           /* zju           */
  n_dbl += nlyr * numu;                        /* zgu           */
  n_dbl += nlyr * nstr + nlyr * nstr;          /* zz, zzg       */
  n_pair += ntau;                              /* fl            */
  n_pair += nlyr * nstr;                       /* plk           */
  n_pair += (nstr / 2) * (nstr / 2);           /* ab            */
  n_pair += nlyr;                              /* xr            */
  n_pair += nstr;                              /* psi           */
  n_pair += nlyr * nstr;                       /* xb            */
  n_pair += nstr;                              /* zbs           */
  n_pair += nlyr * nstr;                       /* zbeamsp       */
  n_pair += nstr;                              /* zee           */
  n_pair += nlyr * numu;                       /* zu            */
  n_trip += nlyr * numu;                       /* zbu           */
  nblocks += 57;

  /* --- c_new_intensity_correction / prep_double_scat_integr ---
   * (nf is hard-coded to 100 there) */
  if (!ds->flag.old_intensity_correction) {
    n_dbl += 100 * ntau;                       /* mu_eq         */
    n_dbl += ntau;                             /* norm_phas     */
    n_int += 100 * ntau;                       /* neg_phas      */
    n_dbl += nphase * ntau;                    /* phas2         */
    n_dbl += nlyr;                             /* phasr         */
    n_dbl += nphase + 1;                       /* f_phas2_abs   */
    nblocks += 6;
  }

  size_t bytes = n_dbl * sizeof(double) + n_int * sizeof(int) +
                 n_pair * sizeof(disort_pair) + n_trip * sizeof(disort_triplet) +
                 n_rad * sizeof(disort_radiant);
  bytes += 64;           /* brdf spec structs */
  bytes += nblocks * 8;  /* worst-case 8-byte alignment padding per pmalloc */
  bytes += 16;           /* pool state at the slice head */

  bytes += bytes / 3;                    /* 1.3x slack */
  bytes = (bytes + 255) & ~(size_t)255;  /* round up to 256 */
  return bytes;
}

/*============================= c_disort() ==============================*/

/*-------------------------------------------------------------------------------*
 * Plane-parallel discrete ordinates radiative transfer program                  *
 * C version                                                                     *
 * Fortran ftp site: ftp://climate.gsfc.nasa.gov/pub/wiscombe/Multiple_Scatt/    *
 *-------------------------------------------------------------------------------*

  Calling Tree (omitting calls to c_errmsg, c_dbl_vector, c_int_vector, c_free_dbl_vector):

  c_disort-+-c_self_test-+-c_disort_state_alloc
         |               +-c_disort_out_alloc
         |               +-c_disort_state_free
         |               +-c_disort_out_free
         +-c_check_inputs-+-(c_write_bad_var)
         |                +-c_dref
         +-c_disort_set-+-c_gaussian_quadrature
         +-c_print_inputs
         +-c_albtrans-+-c_legendre_poly
         |            +-c_solve_eigen-+-c_asymmetric_matrix
         |            +-c_interp_eigenvec
         |            +-c_set_matrix
         |            +-(c_sgbco)
         |            +-c_solve1-+-(c_sgbsl)
         |            +-c_atltrin
         |            +-c_albtrans_spherical
         |            +-c_print_albtrans
         +-c_planck_func1
         +-c_legendre_poly
         +-c_surface_bidir-+-c_gaussian_quadrature
         |                 +-c_bidir_reflectivity
         +-c_solve_eigen-+-c_asymmetric_matrix
	 +-c_set_coefficients_beam_source
	 +-c_interp_coefficients_beam_source
         +-c_upbeam_pseudo_spherical-+-(c_sgeco)
         |                           +-(c_sgesl)
         +-c_upbeam-+-(c_sgeco)
         |          +-(c_sgesl)
         +-c_upbeam_general_source-+-(c_sgeco)
         |                         +-(c_sgesl)
         +-c_upisot-+-(c_sgeco)
         |          +-(c_sgesl)
         +-c_interp_eigenvec
         +-c_interp_source
         +-c_set_matrix
         +-c_solve0-+-(c_sgbco)
         |          +-(c_sgbsl)
         +-c_fluxes
         +-c_user_intensities
         +-c_intensity_components
         +-c_print_avg_intensities
         +-c_ratio
         +-c_intensity_correction-+-c_single_scat
         |                        +-c_secondary_scat-+-c_xi_func
         +-c_new_intensity_correction-+-c_single_scat
         |                            +-prep_double_scat_integr
         |                            +-c_new_secondary_scat-+-c_xi_func
         |                            +-calc_phase_squared
         +-c_print_intensities

 +-------------------------------------------------------------------+

  Index conventions (for all loops and all variable descriptions):

  iu       :  for user polar angles
  iq,jq,kq :  for computational polar angles ('quadrature angles')
  iq/2     :  for half the computational polar angles (just the ones in either 0-90 degrees, or 90-180 degrees)
  j        :  for user azimuthal angles
  k,l      :  for Legendre expansion coefficients or, alternatively, subscripts of associated Legendre polynomials
  lu       :  for user levels
  lc       :  for computational layers (each having a different single-scatter albedo and/or phase function)
  lev      :  for computational levels
  mazim    :  for azimuthal components in Fourier cosine expansion of intensity and phase function

 +------------------------------------------------------------------+

               I N T E R N A L    V A R I A B L E S

   AMB(iq/2,iq/2)....First  matrix factor in reduced eigenvalue problem of eqs. SS(12), STWJ(8E), STWL(23f) (used only in solve_eigen);
                     ab[].zero (see cdisort.h)
   APB(iq/2,iq/2)....Second matrix factor in reduced eigenvalue problem of eqs. SS(12), STWJ(8E), STWL(23f) (used only in solve_eigen);
                     ab[].one (see cdisort.h)
   ARRAY(iq,iq)......Scratch matrix for solve_eigen(), upbeam() and upisot()
                     (see each subroutine for definition)
   B()...............Right-hand side vector of eq. SC(5) going into SOLVE0,1;
                     returns as solution vector vector  L, the constants of integration
   BDR(iq/2,0:iq/2)..Bottom-boundary bidirectional reflectivity for a given azimuthal component.  First index always
                     refers to a computational angle.  Second index: if zero, refers to incident beam angle UMU0;
                     if non-zero, refers to a computational angle.
   BEM(iq/2).........Bottom-boundary directional emissivity at computational angles.
   bplanck...........Intensity emitted from bottom boundary
   callnum...........Number of surface calls
   CBAND()...........Matrix of left-hand side of the linear system eq. SC(5), scaled by eq. SC(12);
                     in banded form required by LINPACK solution routines
   CC(iq,iq).........C-sub-IJ in eq. SS(5)
   CH(lc)............The Chapman-factor to correct for pseudo-spherical geometry in the direct beam.
   CHTAU(lc).........The optical depth in spherical geometry.
   CMU(iq)...........Computational polar angles (Gaussian)
   CWT(iq)...........Quadrature weights corresponding to CMU
   corint............When set TRUE, correct intensities for delta-scaling effects (see Nakajima and Tanaka, 1988).
                     When FALSE, intensities are not corrected. In general, CORINT should be set true when beam
                     source is present (FBEAM is not zero) and DELTAM is TRUE in a problem including scattering.
                     However, execution is faster when CORINT is FALSE, and intensities outside the aureole may still be
                     accurate enough.  When CORINT is TRUE, it is important to have a sufficiently high order of
                     Legendre approximation of the phase function. This is because the intensities are corrected by
                     calculating the single-scattered radiation, for which an adequate representation of the phase
                     function is crucial.  In case of a low order Legendre approximation of an otherwise highly
                     anisotropic phase function, the intensities might actually be more accurate when corint is FALSE.
                     When only fluxes are calculated (ds->flag.onlyfl is TRUE), or there is no beam source (FBEAM=0.0), or there
                     is no scattering (SSALB =0. for all layers) corint is set FALSE by the code.
   delm0.............Kronecker delta, delta-sub-M0, where M = MAZIM is the number of the Fourier component in the
                     azimuth cosine expansion
   deltam............TRUE,  use delta-M method ( see Wiscombe, 1977 );
                     FALSE, do not use delta-M method. 
                     In general, for a given number of streams, intensities and fluxes will be more accurate for phase functions
                     with a large forward peak if DELTAM is set true. Intensities close to the forward scattering
                     direction are often less accurate, however, when the delta-M method is applied. The intensity deltam
                     correction of Nakajima and Tanaka is used to improve the accuracy of the intensities.
   dither............Small quantity subtracted from single-scattering albedos of unity, in order to avoid using special
                     case formulas;  prevents an eigenvalue of exactly zero from occurring, which would cause an immediate overflow
   DTAUCPR(lc).......Computational-layer optical depths (delta-M-scaled if DELTAM = TRUE, otherwise equal to DTAUC)
   EMU(iu)...........Bottom-boundary directional emissivity at user angles.
   EVAL(iq)..........Temporary storage for eigenvalues of eq. SS(12)
   EVECC(iq,iq)......Complete eigenvectors of SS(7) on return from solve_eigen; stored permanently in  GC
   EXPBEA(lc)........Transmission of direct beam in delta-M optical depth coordinates
   FLYR(lc)..........Separated fraction in delta-M method
   GL(k,lc)..........Phase function Legendre polynomial expansion coefficients, calculated from PMOM by
                     including single-scattering albedo, factor 2K+1, and (if DELTAM=TRUE) the delta-M scaling
   GC(iq,iq,lc)......Eigenvectors at polar quadrature angles, g in eq. SC(1)
   GU(iu,iq,lc)......Eigenvectors interpolated to user polar angles (g  in eqs. SC(3) and S1(8-9), i.e. g without the l factor)
   IPVT(lc*iq).......Integer vector of pivot indices for LINPACK routines
   KK(iq,lc).........Eigenvalues of coeff. matrix in eq. SS(7)
   kconv.............Counter in azimuth convergence test
   LAYRU(lu).........Computational layer in which user output level UTAU(LU) is located
   LL(iq,lc).........Constants of integration L in eq. SC(1), obtained by solving scaled version of eq. SC(5)
   lyrcut............TRUE, radiation is assumed zero below layer ncut because of almost complete absorption
   naz...............Number of azimuthal components considered
   ncut..............Computational layer number in which absorption optical depth first exceeds ABSCUT
   OPRIM(lc).........Single scattering albedo after delta-M scaling
   pass1.............TRUE on first entry, FALSE thereafter
   PKAG(0:lc)........Integrated Planck function for internal emission
   PRNTU0(l).........logical flag to trigger printing of azimuthally-averaged intensities:
                       l    quantities printed
                      --    ------------------
                       0    azimuthally-averaged intensities at user
                               levels and computational polar angles
                       1    azimuthally-averaged intensities at user
                               levels and user polar angles
   PSI0(iq)..........Sum just after square bracket in  eq. SD(9); psi[].zero (see cdisort.h)
   PSI1(iq)..........Sum in  eq. STWL(31d); psi[].one
   RMU(iu,0:iq)......Bottom-boundary bidirectional reflectivity for a given azimuthal component.  First index always
                     refers to a user angle.  Second index: if zero, refers to incident beam angle UMU0;
                     if non-zero, refers to a computational angle.
   scat_yes..........int, TRUE if scattering, FALSE if not (added to C version)
   TAUC(0:lc)........Cumulative optical depth (un-delta-M-scaled)
   TAUCPR(0:lc)......Cumulative optical depth (delta-M-scaled if DELTAM = TRUE, otherwise equal to TAUC)
   tplanck...........Intensity emitted from top boundary
   UUM(iu,lu)........Expansion coefficients when the intensity (u-super-M) is expanded in Fourier cosine series
                     in azimuth angle
   U0C(iq,lu)........Azimuthally-averaged intensity at quadrature angle
   U0U(iu,lu)........If ds->flag.onlyfl = FALSE, azimuthally-averaged intensity at user angles and user levels
                     If ds->flag.onlyfl = TRUE, azimuthally-averaged intensity at computational
                     (Gaussian quadrature) angles and user levels; the corresponding quadrature angle cosines are
                     returned in UMU.
   UTAUPR(lu)........Optical depths of user output levels in delta-M coordinates; equal to UTAU(LU) if no delta-M
   WK(iq)............Scratch array
   XR0(lc)...........X-sub-zero in expansion of thermal source function preceding eq. SS(14)(has no mu-dependence); b-sub-zero in eq. STWL(24d)
   XR1(lc)...........X-sub-one in expansion of thermal source function; see eqs. SS(14-16); b-sub-one in STWL(24d)
   YLM0(l)...........Normalized associated Legendre polynomial of subscript L at the beam angle (not saved
                     as function of superscipt M)
   YLMC(l,iq)........Normalized associated Legendre polynomial of subscript L at the computational angles
                     (not saved as function of superscipt M)
   YLMU(l,iu)........Normalized associated Legendre polynomial of subscript L at the user angles
                     (not saved as function of superscipt M)
   Z()...............scratch array used in solve0(), albtrans() to solve a linear system for the constants of integration
   Z0(iq)............Solution vectors Z-sub-zero of eq. SS(16); zee[].zero (see cdisort.h)
   Z1(iq)............Solution vectors Z-sub-one  of eq. SS(16); zee[].one
   Z0U(iu,lc)........Z-sub-zero in eq. SS(16) interpolated to user angles from an equation derived from SS(16); zu[].zero (see cdisort.h)
   Z1U(iu,lc)........Z-sub-one  in eq. SS(16) interpolated to user angles from an equation derived from SS(16); zu[].one
   ZBEAM(iu,lc)......Particular solution for beam source
   ZGU(iu,lc)........General source function interpolated to user angles
   ZJ(iq)............Right-hand side vector  X-sub-zero in eq. SS(19), also the solution vector
                     Z-sub-zero after solving that system
   ZJG(iq)...........Right-hand side vector  X-sub-zero in eq. KS(10), also the solution vector
                     Z-sub-zero after solving that system for a general source constant over a layer
   ZZ(iq,lc).........Permanent storage for the beam source vectors ZJ
   ZZG(iq,lc)........Permanent storage for the beam source vectors ZJG
   ZPLK0(iq,lc)......Permanent storage for the thermal source vectors plk[].zero obtained by solving eq. SS(16)
   ZPLK1(iq,lc)......Permanent storage for the thermal source vectors plk[].one  obtained by solving eq. SS(16)

*/ 

DISPATCH_MACRO inline int c_disort(disort_state  *ds,
	      disort_output *out,
        emission_func_t emi_func)
{
  int
    self_tested = 1;  // cli: disable self-test by default
  int
    prntu0[2],
    corint,deltam,scat_yes,compare,lyrcut,needdeltam,
    iq,iu,j,kconv,l,lc,lev,lu,mazim,naz,ncol,ncos,ncut,nn;
  int
    callnum=1;
  int
    *ipvt,
    *layru;
  double
    angcos,azerr,azterm,bplanck,cosphi,delm0,
    sgn,tplanck;
  double
    *array,*b,*bdr,*bem,*cband,*cc,*ch,*chtau,
    *cmu,*cwt, *dtaucpr,*emu,*eval,*evecc,*expbea,
    *flyr,*gc,*gl,*gu,*kk,*ll,
    *oprim,*phasa,*phast,*phasm,*phirad,*pkag,
    *rmu,*tauc,*taucpr,*u0c,*utaupr,*uum,
    *wk,*xba,*ylm0,*ylmc,*ylmu,
    *z,*zbeam,
    *zbeama,zbsa=0,*zj,*zjg,*zju,*zgu,*zz,*zzg;
  disort_pair
    *ab,*fl,*plk,*xr,*psi,*xb,*zbeamsp,*zbs,*zee,*zu;
  disort_triplet
    *zbu;
  const double
    dither = 100.*DBL_EPSILON;


  /* Set these here to ensure that memory is correctly allocated. */
  if (!ds->flag.usrtau) {
    ds->ntau = ds->nlyr+1;
  }
  if ( ((!ds->flag.usrang) || ds->flag.onlyfl)  && ( (!ds->flag.ibcnd) == SPECIAL_BC)) {
    ds->numu = ds->nstr;
  }
  if (ds->flag.usrang && ds->flag.ibcnd == SPECIAL_BC) {
    ds->numu *= 2;
  }

  /* ipvt and layru were stack VLAs in cdisort.c; VLAs are not valid in
   * C++/device code, so they are pool-allocated instead. */
  ipvt  = (int *)pmalloc(ds->nstr*ds->nlyr*sizeof(int));
  layru = (int *)pmalloc(ds->ntau*sizeof(int));

  if (self_tested == -1) {
    int
      prntu0_test[2] = {FALSE,FALSE};
    disort_state
      ds_test;
    disort_output
      out_test;
    /*
     * Set input values for self-test.
     * Be sure self_test() sets all print flags off.
     */
    self_tested = 0;
    compare     = FALSE;
    c_self_test(compare,prntu0_test,&ds_test,&out_test);
    c_disort(&ds_test,&out_test,emi_func);
  }

  /*
   * Determine whether there is scattering or not
   */
  scat_yes = FALSE;
  for (lc = 1; lc <= ds->nlyr; lc++) {
    if (SSALB(lc) > 0.) {
      scat_yes = TRUE;
      break;
    }
  }

  /* 
   * Turn on delta-M tranformation
   */
  deltam = TRUE;

  /* delta-M scaling makes only sense if phase function has more
   * moments than streams
   */
  needdeltam = FALSE;
  if( deltam==TRUE ) {
    for (lc=1; lc<=ds->nlyr; lc++)
      if ( PMOM(ds->nstr,lc) != 0.0 )
	needdeltam = TRUE;
    if (needdeltam==FALSE)
      deltam=FALSE;
  }

  /*
   * Turn off intensity correction when only fluxes are calculated, there
   * is no beam source, no scattering, or delta-M transformation is not applied
   */
  corint = ds->flag.intensity_correction;
  if (ds->flag.onlyfl || ds->bc.fbeam == 0. || !scat_yes || !deltam) 
    corint = FALSE;

  prntu0[0] = FALSE;
  prntu0[1] = FALSE;

  /*
   * Allocate zeroed memory
   */
  tauc = c_dbl_vector(0,ds->nlyr,"tauc");

  for (lc = 1; lc <= ds->nlyr; lc++) {
    if(SSALB(lc) == 1.) {
      SSALB(lc) = 1.-dither;
    }
    TAUC(lc) = TAUC(lc-1)+DTAUC(lc);
  }

  /* Check input dimensions and variables */
  int err = c_check_inputs(ds,scat_yes,deltam,corint,tauc,callnum);
  if (err) {
    pfree(tauc);
    pfree(ipvt);
    pfree(layru);
    return err;
  }

  /*-------------------------------------------------------------------------------------------*
   * Special case for getting albedo and transmissivity of medium for many beam angles at once *
   *-------------------------------------------------------------------------------------------*/

  if (ds->flag.ibcnd == SPECIAL_BC) {
    /* 
     * Allocate zeroed memory
     */
    array    = c_dbl_vector(0,ds->nstr*ds->nstr-1,"array");
    b        = c_dbl_vector(0,ds->nstr*ds->nlyr-1,"b");
    bdr      = c_dbl_vector(0,((ds->nstr/2)+1)*(ds->nstr/2)-1,"bdr");
    cband    = c_dbl_vector(0,ds->nstr*ds->nlyr*(9*(ds->nstr/2)-2)-1,"cband");
    ch       = c_dbl_vector(0,ds->nlyr-1,"ch");
    chtau    = c_dbl_vector(0,(2*ds->nlyr+1)-1,"chtau");
    cc       = c_dbl_vector(0,ds->nstr*ds->nstr-1,"cc");
    cmu      = c_dbl_vector(0,ds->nstr-1,"cmu");
    cwt      = c_dbl_vector(0,ds->nstr-1,"cwt");
    dtaucpr  = c_dbl_vector(0,ds->nlyr-1,"dtaucpr");
    eval     = c_dbl_vector(0,(ds->nstr/2)-1,"eval");
    evecc    = c_dbl_vector(0,ds->nstr*ds->nstr-1,"evecc");
    expbea   = c_dbl_vector(0,ds->nlyr,"expbea");
    flyr     = c_dbl_vector(0,ds->nlyr-1,"flyr");
    gc       = c_dbl_vector(0,ds->nlyr*ds->nstr*ds->nstr-1,"gc");
    gl       = c_dbl_vector(0,ds->nlyr*(ds->nstr+1),"gl");
    gu       = c_dbl_vector(0,ds->nlyr*ds->nstr*ds->numu-1,"gu");
    kk       = c_dbl_vector(0,ds->nlyr*ds->nstr-1,"kk");
    ll       = c_dbl_vector(0,ds->nlyr*ds->nstr-1,"ll");
    oprim    = c_dbl_vector(0,ds->nlyr-1,"oprim");
    taucpr   = c_dbl_vector(0,ds->nlyr,"taucpr");
    utaupr   = c_dbl_vector(0,ds->ntau-1,"utaupr");
    wk       = c_dbl_vector(0,ds->nstr-1,"wk");
    ylmc     = c_dbl_vector(0,ds->nstr*(ds->nstr+1)-1,"ylmc");
    ylmu     = c_dbl_vector(0,(ds->numu)*(ds->nstr+1)-1,"ylmu");
    z        = c_dbl_vector(0,ds->nstr*ds->nlyr-1,"z");

    ab       = (disort_pair *)pcalloc((ds->nstr/2)*(ds->nstr/2),sizeof(disort_pair));
    if (!ab) {
      c_errmsg("disort alloc error for ab", DS_ERROR);
    }
    /*
     * Zero output arrays
     */
    if (!ds->flag.usrtau) {
      memset(ds->utau,0,ds->ntau*sizeof(double));
    }
    if (!ds->flag.usrang || ds->flag.onlyfl) {
      memset(ds->umu,0,(ds->numu+1)*sizeof(double));
    }
    memset(out->rad,   0,ds->ntau*sizeof(disort_radiant));
    memset(out->albmed,0,ds->numu*sizeof(double));
    memset(out->trnmed,0,ds->numu*sizeof(double));
    if (ds->flag.onlyfl == FALSE) {
      memset(out->uu,0,ds->numu*ds->ntau*ds->nphi*sizeof(double));
    }

    /* Perform various setup operations */
    c_disort_set(ds,ch,chtau,cmu,cwt,deltam,dtaucpr,expbea,flyr,gl,layru,&lyrcut,&ncut,&nn,&corint,oprim,tauc,taucpr,utaupr,emi_func);

    /*  Print input information */
    if(ds->flag.prnt[0]) {
      c_print_inputs(ds,dtaucpr,scat_yes,deltam,corint,flyr,lyrcut,oprim,tauc,taucpr);
    }

    c_albtrans(ds,out,ab,array,b,bdr,cband,cc,cmu,cwt,dtaucpr,eval,evecc,gl,gc,gu,ipvt,kk,ll,nn,taucpr,ylmc,ylmu,z,wk);

    callnum++;

    /*
     * Free allocated memory
     */
    pfree(array), pfree(b),    pfree(bdr), pfree(cband),  pfree(cc),    pfree(ch);
    pfree(chtau), pfree(cmu),  pfree(cwt), pfree(dtaucpr),pfree(eval),  pfree(evecc);
    pfree(expbea),pfree(flyr), pfree(gc),  pfree(gl),     pfree(gu),    pfree(kk);
    pfree(ll),    pfree(oprim),pfree(tauc),pfree(taucpr), pfree(utaupr),pfree(wk);
    pfree(ylmc),  pfree(ylmu), pfree(z),   pfree(ab);
    pfree(ipvt),  pfree(layru);

    return 0;
  }

  /*--------------*
   * General case *
   *--------------*/

  /* 
   * Allocate zeroed memory
   */
  array   = c_dbl_vector(0,ds->nstr*ds->nstr-1,"array");
#ifdef __CUDA_ARCH__
  if (ds->flag.onlyfl) {
    b = c_dbl_vector_uninitialized(0,ds->nstr*ds->nlyr-1,"b");
  }
  else
#endif
  {
    b = c_dbl_vector(0,ds->nstr*ds->nlyr-1,"b");
  }
  bdr     = c_dbl_vector(0,((ds->nstr/2)+1)*(ds->nstr/2)-1,"bdr");
  bem     = c_dbl_vector(0,(ds->nstr/2)-1,"bem");
  cband = c_dbl_vector(0,ds->nstr*ds->nlyr*(9*(ds->nstr/2)-2)-1,"cband");
  cc      = c_dbl_vector(0,ds->nstr*ds->nstr-1,"cc");
  ch      = c_dbl_vector(0,ds->nlyr-1,"ch");
  chtau   = c_dbl_vector(0,(2*ds->nlyr+1)-1,"chtau");
  cmu     = c_dbl_vector(0,ds->nstr-1,"cmu");
  cwt     = c_dbl_vector(0,ds->nstr-1,"cwt");
  dtaucpr = c_dbl_vector(0,ds->nlyr-1,"dtaucpr");
  emu     = c_dbl_vector(0,ds->numu-1,"emu");
  eval    = c_dbl_vector(0,(ds->nstr/2)-1,"eval");
  evecc   = c_dbl_vector(0,ds->nstr*ds->nstr-1,"evecc");
  expbea  = c_dbl_vector(0,ds->nlyr,"expbea");
  flyr    = c_dbl_vector(0,ds->nlyr,"flyr");    // We need at least one element
  gc      = c_dbl_vector(0,ds->nlyr*ds->nstr*ds->nstr-1,"gc");
  gl      = c_dbl_vector(0,ds->nlyr*(ds->nstr+1),"gl");
  gu      = c_dbl_vector(0,ds->nlyr*ds->nstr*ds->numu-1,"gu");
  kk      = c_dbl_vector(0,ds->nlyr*ds->nstr-1,"kk");
  ll      = c_dbl_vector(0,ds->nlyr*ds->nstr-1,"ll");
  oprim   = c_dbl_vector(0,ds->nlyr-1,"oprim");
  phasa   = c_dbl_vector(0,ds->nlyr-1,"phasa");
  phast   = c_dbl_vector(0,ds->nlyr-1,"phast");
  phasm   = c_dbl_vector(0,ds->nlyr-1,"phasm");
  if (ds->nphi > 0) {
    phirad = c_dbl_vector(0,ds->nphi-1,"phirad");
  }
  else {
    phirad = NULL;
  }
  pkag   = c_dbl_vector(0,ds->nlyr,"pkag");
  rmu    = c_dbl_vector(0,((ds->nstr/2)+1)*ds->numu-1,"rmu");
  taucpr = c_dbl_vector(0,ds->nlyr,"taucpr");
#ifdef __CUDA_ARCH__
  if (ds->flag.onlyfl) {
    u0c = c_dbl_vector_uninitialized(0,ds->ntau*ds->nstr-1,"u0c");
  }
  else
#endif
  {
    u0c = c_dbl_vector(0,ds->ntau*ds->nstr-1,"u0c");
  }
  utaupr = c_dbl_vector(0,ds->ntau-1,"utaupr");
  uum    = c_dbl_vector(0,ds->ntau*ds->numu-1,"uum");
  wk     = c_dbl_vector(0,ds->nstr-1,"wk");
  xba    = c_dbl_vector(0,ds->nlyr,"xba");
  ylm0   = c_dbl_vector(0,ds->nstr,"ylm0");
  ylmc   = c_dbl_vector(0,ds->nstr*(ds->nstr+1)-1,"ylmc");
  ylmu   = c_dbl_vector(0,ds->numu*(ds->nstr+1)-1,"ylmu");
  z      = c_dbl_vector(0,ds->nstr*ds->nlyr-1,"z");
  zbeam  = c_dbl_vector(0,ds->nlyr*ds->numu-1,"zbeam");
  zbeama = c_dbl_vector(0,ds->nlyr,"zbeama");
  zj     = c_dbl_vector(0,ds->nstr-1,"zj");
  zjg    = c_dbl_vector(0,ds->nstr-1,"zjg");
  zju    = c_dbl_vector(0,ds->numu,"zju");
  zgu    = c_dbl_vector(0,ds->nlyr*ds->numu-1,"zgu");
  zz     = c_dbl_vector(0,ds->nlyr*ds->nstr-1,"zz");
  zzg    = c_dbl_vector(0,ds->nlyr*ds->nstr-1,"zzg");
  /*
   * Using C structures facilitates cache-aware memory allocation, which can reduce
   * cache misses and potentially speed up computer execution.
   */
#ifdef __CUDA_ARCH__
  if (ds->flag.onlyfl) {
    fl = (disort_pair *)pmalloc(ds->ntau*sizeof(disort_pair));
  }
  else
#endif
  {
    fl = (disort_pair *)pcalloc(ds->ntau,sizeof(disort_pair));
  }
  if (!fl) c_errmsg("disort alloc error for fl", DS_ERROR);
  plk    = (disort_pair *)pcalloc(ds->nlyr*ds->nstr,sizeof(disort_pair));         if (!plk)     c_errmsg("disort alloc error for plk",DS_ERROR);
  ab     = (disort_pair *)pcalloc((ds->nstr/2)*(ds->nstr/2),sizeof(disort_pair)); if (!ab)      c_errmsg("disort alloc error for ab", DS_ERROR);
  xr     = (disort_pair *)pcalloc(ds->nlyr,sizeof(disort_pair));                  if (!xr)      c_errmsg("disort alloc error for xr", DS_ERROR);
  psi    = (disort_pair *)pcalloc(ds->nstr,sizeof(disort_pair));                  if (!psi)     c_errmsg("disort alloc error for psi",DS_ERROR);
  xb     = (disort_pair *)pcalloc(ds->nlyr*ds->nstr,sizeof(disort_pair));         if (!xb)      c_errmsg("disort alloc error for xb",DS_ERROR);
  zbs    = (disort_pair *)pcalloc(ds->nstr,sizeof(disort_pair));                  if (!zbs)     c_errmsg("disort alloc error for zbs",DS_ERROR);
  zbeamsp= (disort_pair *)pcalloc(ds->nlyr*ds->nstr,sizeof(disort_pair));         if (!zbeamsp) c_errmsg("disort alloc error for zbeamsp",DS_ERROR);
  zee    = (disort_pair *)pcalloc(ds->nstr,sizeof(disort_pair));                  if (!zee)     c_errmsg("disort alloc error for zee",DS_ERROR);
  zu     = (disort_pair *)pcalloc(ds->nlyr*ds->numu,sizeof(disort_pair));         if (!zu)      c_errmsg("disort alloc error for zu", DS_ERROR);

  zbu    = (disort_triplet *)pcalloc(ds->nlyr*ds->numu,sizeof(disort_triplet));   if (!zbu)     c_errmsg("disort alloc error for zbu", DS_ERROR);

  /*
   * Zero output arrays
   */ 
  if (!ds->flag.usrtau) {
    memset(ds->utau,0,ds->ntau*sizeof(double));
  }
  if (!ds->flag.usrang || ds->flag.onlyfl) {
    memset(ds->umu,0,(ds->numu)*sizeof(double));
  }
  memset(out->rad,0,ds->ntau*sizeof(disort_radiant));
  if (ds->flag.onlyfl == FALSE) {
    memset(out->uu,0,ds->numu*ds->ntau*ds->nphi*sizeof(double));
  }

  /* Perform various setup operations */
  c_disort_set(ds,ch,chtau,cmu,cwt,deltam,dtaucpr,expbea,flyr,gl,layru,&lyrcut,&ncut,&nn,&corint,oprim,tauc,taucpr,utaupr,emi_func);


  /*  Print input information */
  if(ds->flag.prnt[0]) {
    c_print_inputs(ds,dtaucpr,scat_yes,deltam,corint,flyr,lyrcut,oprim,tauc,taucpr);
  }

  /*
   * Calculate Planck functions
   */
  if (!ds->flag.planck) {
    bplanck = 0.;
    tplanck = 0.;
  }
  else {
    tplanck = emi_func(ds->wvnmlo,ds->wvnmhi,ds->bc.ttemp)*ds->bc.temis;
    bplanck = emi_func(ds->wvnmlo,ds->wvnmhi,ds->bc.btemp);
    for (lev = 0; lev <= ds->nlyr; lev++) {
      PKAG(lev) = emi_func(ds->wvnmlo,ds->wvnmhi,TEMPER(lev));
    }
  }

  /*
   *--------  BEGIN LOOP TO SUM AZIMUTHAL COMPONENTS OF INTENSITY  ---------
   *          (eq STWJ 5, STWL 6)
   */
  kconv = 0;
  naz   = ds->nstr-1;

  /*
   * Azimuth-independent case
   */
  if (ds->bc.fbeam == 0.                         || 
      fabs(1.-ds->bc.umu0) < 1.e-5               || 
      ds->flag.onlyfl                            || 
      (ds->numu == 1 && fabs(1.-UMU(1)) < 1.e-5) || 
      (ds->numu == 1 && fabs(1.+UMU(1)) < 1.e-5) || 
      (ds->numu == 2 && fabs(1.+UMU(1)) < 1.e-5 && fabs(1.-UMU(2)) < 1.e-5)) {
    naz = 0;
  }

  for (mazim = 0; mazim <= naz; mazim++) {
    if (mazim == 0) {
      delm0 = 1.;
    }
    else {
      delm0 = 0.;
    }

    /*
     * Get normalized associated Legendre polynomials for
     *   (a) incident beam angle cosine
     *   (b) computational and user polar angle cosines
     */
    if (ds->bc.fbeam > 0.) {
      ncos   = 1;
      angcos = -ds->bc.umu0;
      c_legendre_poly(ncos,mazim,ds->nstr,ds->nstr-1,&angcos,ylm0);
    }

    if (!ds->flag.onlyfl && ds->flag.usrang) {
      c_legendre_poly(ds->numu,mazim,ds->nstr,ds->nstr-1,ds->umu,ylmu);
    }
    c_legendre_poly(nn,mazim,ds->nstr,ds->nstr-1,cmu,ylmc);

    /*
     * Get normalized associated Legendre polynomials with negative arguments from those with
     * positive arguments; Dave/Armstrong eq. (15), STWL(59)
     */
    sgn = -1.;
    for (l = mazim; l <= ds->nstr-1; l++) {
      sgn *= -1.;
      for (iq = nn+1; iq <= ds->nstr; iq++) {
        YLMC(l,iq) = sgn*YLMC(l,iq-nn);
      }
    }

    /*
     * Specify users bottom reflectivity and emissivity properties
     */
    if (!lyrcut) {
      c_surface_bidir(ds, delm0, cmu, mazim, nn, bdr, emu, bem, rmu,
		      callnum);
    }

    /*--------------  BEGIN LOOP ON COMPUTATIONAL LAYERS  ------------*/
    for (lc = 1; lc <= ncut; lc++) {
      /*
       * Solve eigenfunction problem in eq. STWJ(8B), STWL(23f); return eigenvalues and eigenvectors
       */
      c_solve_eigen(ds,lc,ab,array,cmu,cwt,gl,mazim,nn,ylmc,cc,evecc,eval,kk,gc,wk);
      /*
       * Calculate particular solutions of eq. SS(18), STWL(24a) for incident beam source
       */
      if (ds->bc.fbeam > 0.) {
	if ( ds->flag.spher == TRUE ) {
	  /* Pseudo-spherical approach */
	  c_set_coefficients_beam_source(ds,ch,chtau,cmu,delm0,ds->bc.fbeam,
					 gl,lc,mazim,ds->nstr,
					 taucpr,xba,xb,ylm0,ylmc,zj);
	  
	  if ( ds->flag.usrang == TRUE  ) {
	    /* Get coefficients at umu for pseudo-spherical source */
	    c_interp_coefficients_beam_source(ds,chtau,delm0,ds->bc.fbeam,
					      gl,lc,mazim,ds->nstr,
					      ds->numu,taucpr,zbu,
					      xba,zju,ylm0,ylmu);
	  }
	  c_upbeam_pseudo_spherical(ds,lc,array,cc,cmu,ipvt,nn,wk,
				    xb,xba,zbs,&zbsa,zbeamsp,zbeama);
	}
	else {
	  /* Plane-parallel version */
	  c_upbeam(ds,lc,array,cc,cmu,delm0,gl,ipvt,mazim,nn,wk,ylm0,ylmc,zj,zz);
	}
      }

      /*
       * Calculate particular solutions of eq. SS(18), STWL(24a), KS(5) for 
       * general user specified source.
       */
      if (ds->flag.general_source) {
	c_upbeam_general_source(ds,lc,mazim,array,cc,ipvt,nn,wk,zjg,zzg);
      }

      /*
       * Calculate particular solutions of eq. SS(15), STWL(25) for thermal emission source
       */
      if (ds->flag.planck && mazim == 0) {
        XR1(lc) = 0.;
        if (DTAUCPR(lc) > 1e-4) { /* fix by RPB, caused problems in make check AVHRR CH4/5 */
          XR1(lc) = (PKAG(lc)-PKAG(lc-1))/DTAUCPR(lc);
        }
        XR0(lc) = PKAG(lc-1)-XR1(lc)*TAUCPR(lc-1);
        c_upisot(ds,lc,array,cc,cmu,ipvt,nn,oprim,wk,xr,zee,plk);
      }

      if (!ds->flag.onlyfl && ds->flag.usrang) {
        /*
         * Interpolate eigenvectors to user angles
         */
        c_interp_eigenvec(ds,lc,cwt,evecc,gl,gu,mazim,nn,wk,ylmc,ylmu);
        /*
         * Interpolate source terms to user angles
         */
        c_interp_source(ds,lc,cwt,delm0,gl,mazim,oprim,ylm0,ylmc,ylmu,
			psi,xr,zee,zj,zjg,zbeam,zbu,zbs,zbsa,zgu,zu);
      }
    }
    /*-------------------  END LOOP ON COMPUTATIONAL LAYERS  ----------------*/

    /*
     *
     * Set coefficient matrix of equations combining boundary and layer interface conditions
     */
    c_set_matrix(ds,bdr,cband,cmu,cwt,delm0,dtaucpr,gc,kk,lyrcut,&ncol,ncut,taucpr,wk);

    /*
     * Solve for constants of integration in homogeneous solution (general boundary conditions)
     */
    c_solve0(ds,b,bdr,bem,bplanck,cband,cmu,cwt,expbea,ipvt,ll,lyrcut,
	     mazim,ncol,ncut,nn,tplanck,taucpr,z,zbeamsp,zbeama,zz,zzg,plk);

    /*
     * Compute upward and downward fluxes
     */
    if (mazim == 0) {
      c_fluxes(ds,out,ch,cmu,cwt,gc,kk,layru,ll,lyrcut,ncut,nn,PRNTU0(1),
	       taucpr,utaupr,xr,zbeamsp,zbeama,zz,zzg,plk,fl,u0c);
    }

    if (ds->flag.onlyfl) {
      /*
       * Save azimuthal-avg intensities at quadrature angles when the output
       * buffer is allocated. CUDA flux-only calls do not expose this output.
       */
      if (out->u0u != NULL) {
        for (lu = 1; lu <= ds->ntau; lu++) {
          for (iq = 1; iq <= ds->nstr; iq++) {
            U0U(iq,lu) = U0C(iq,lu);
          }
        }
      }
      break;
    }

    memset(uum,0,ds->numu*ds->ntau*sizeof(double));

    if (ds->flag.usrang) {
      /*
       * Compute azimuthal intensity components at user angles
       */
      c_user_intensities(ds,bplanck,cmu,cwt,delm0,dtaucpr,emu,expbea,
			 gc,gu,kk,layru,ll,lyrcut,mazim,
			 ncut,nn,rmu,taucpr,tplanck,utaupr,wk,
			 zbu,zbeam,zbeamsp,
			 zbeama,zgu,zu,zz,zzg,plk,uum);
    }
    else {
      /*
       * Compute azimuthal intensity components at quadrature angles
       */
      c_intensity_components(ds,gc,kk,layru,ll,lyrcut,mazim,ncut,nn,taucpr,utaupr,zz,plk,uum);
    }

    if (mazim == 0) {
      /*
       * Save azimuthally averaged intensities
       */
      for (lu = 1; lu <= ds->ntau; lu++) {
        for (iu = 1; iu <= ds->numu; iu++) {
          U0U(iu,lu) = UUM(iu,lu);
          for (j = 1; j <= ds->nphi; j++) {
            UU(iu,lu,j) = UUM(iu,lu);
          }
        }
      }

      if ( ds->flag.output_uum)
	for (lu = 1; lu <= ds->ntau; lu++) 
	  for (iu = 1; iu <= ds->numu; iu++)
            OUT_UUM(iu,lu,mazim) = UUM(iu,lu);
      
      /*
       * Print azimuthally averaged intensities at user angles
       */
      if (PRNTU0(2)) {
        c_print_avg_intensities(ds,out);
      }

      if (naz > 0) {
        memset(phirad,0,ds->nphi*sizeof(double));
        for (j = 1; j <= ds->nphi; j++) {
          PHIRAD(j) = (PHI(j)-ds->bc.phi0)*DEG;
        }
      }
    }
    else {
      /*
       * Increment intensity by current azimuthal component (Fourier cosine series);  eq SD(2), STWL(6)
       */
      azerr = 0.;
      for (j = 1; j <= ds->nphi; j++) {
        cosphi = cos((double)mazim*PHIRAD(j));
        for (lu = 1; lu <= ds->ntau; lu++) {
          for (iu = 1; iu <= ds->numu; iu++) {
            azterm       = UUM(iu,lu)*cosphi;
            UU(iu,lu,j) += azterm;
            azerr        = MAX(azerr,c_ratio(fabs(azterm),fabs(UU(iu,lu,j))));
          }
        }
      }
      if ( ds->flag.output_uum)
	for (lu = 1; lu <= ds->ntau; lu++) 
	  for (iu = 1; iu <= ds->numu; iu++) 
            OUT_UUM(iu,lu,mazim) = UUM(iu,lu);

      if(azerr <= ds->accur) {
        kconv++;
      }
      if (kconv >= 2) {
        break;
      }
    }
  }
  /*--------------  END LOOP ON AZIMUTHAL COMPONENTS  ----------------*/


  for (iu = 1; iu <= ds->numu; iu++) {
    lu = ds->ntau;
    j =  1;
  }
  if (corint) {
    /*
     * Apply Nakajima/Tanaka intensity corrections
     */
    if (!ds->flag.old_intensity_correction && self_tested == 1) {
      if (ds->flag.quiet==VERBOSE)
	fprintf(stderr,"Using new intensity correction, with phase functions\n");
      c_new_intensity_correction(ds,out,dither,flyr,layru,lyrcut,ncut,oprim,phasa,phast,phasm,phirad,tauc,taucpr,utaupr);
    }
    else {
      if (ds->flag.quiet==VERBOSE) 
	fprintf(stderr,"Using original intensity correction, with phase moments\n");
      c_intensity_correction(ds,out,dither,flyr,layru,lyrcut,ncut,oprim,phasa,phast,phasm,phirad,tauc,taucpr,utaupr);
    }
  }

  
  for (iu = 1; iu <= ds->numu; iu++) {
    lu = ds->ntau;
    j =  1;
  }


  if (ds->flag.prnt[2] && !ds->flag.onlyfl) {
    /*
     * Print intensities
     */
    c_print_intensities(ds,out);
  }

  if (self_tested == 0) {
    /*
     * Compare test case results with correct answers and abort if bad
     */
    compare = TRUE;
    c_self_test(compare,prntu0,ds,out);

    self_tested = 1;
  }

  callnum++;

  /* 
   * Free allocated memory
   */
  pfree(ab),pfree(array);
  pfree(b),pfree(bdr),pfree(bem);
  pfree(cband),pfree(cc),pfree(ch),pfree(chtau),pfree(cmu),pfree(cwt);
  pfree(dtaucpr);
  pfree(emu),pfree(eval),pfree(evecc),pfree(expbea);
  pfree(flyr),pfree(fl);
  pfree(gc),pfree(gl),pfree(gu);
  pfree(kk);
  pfree(ll);
  pfree(oprim);
  pfree(phasa),pfree(phast),pfree(phasm),pfree(phirad),pfree(pkag),pfree(plk),pfree(psi);
  pfree(rmu);
  pfree(tauc),pfree(taucpr);
  pfree(u0c),pfree(utaupr),pfree(uum);
  pfree(wk);
  pfree(xb),pfree(xba),pfree(xr);
  pfree(ylm0),pfree(ylmc),pfree(ylmu);
  pfree(z),pfree(zbu),pfree(zbeam),pfree(zbeamsp),
  pfree(zbeama),pfree(zbs),pfree(zj),pfree(zjg),pfree(zju),
  pfree(zgu),pfree(zz),pfree(zzg),pfree(zee),pfree(zu);
  pfree(ipvt),pfree(layru);

  return 0;
}

/*============================= end of c_disort() =======================*/
