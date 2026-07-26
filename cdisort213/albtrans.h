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

/*============================= c_albtrans() =============================*/

/*
   DISORT special case to get only albedo and transmissivity of entire medium as a function of incident beam angle
   (many simplifications because boundary condition is just isotropic illumination, there are no thermal sources, and
   particular solutions do not need to be computed).  See Ref. S2 and references therein for details.
   The basic idea is as follows.  The reciprocity principle leads to the following relationships for a plane-parallel,
   vertically inhomogeneous medium lacking thermal (or other internal) sources:
  
      albedo(theta) = u_0(theta) for unit-intensity isotropic
                       illumination at *top* boundary
       trans(theta) =  u_0(theta) for unit-intensity isotropic
                       illumination at *bottom* boundary
    where

       albedo(theta) = albedo for beam incidence at angle theta
       trans(theta)  = transmissivity for beam incidence at angle theta
       u_0(theta)    = upward azim-avg intensity at top boundary
                       at angle theta

   O U T P U T    V A R I A B L E S:
  
       ALBMED(iu)   Albedo of the medium as a function of incident
                    beam angle cosine UMU(IU)
  
       TRNMED(iu)   Transmissivity of the medium as a function of
                    incident beam angle cosine UMU(IU)

    I N T E R N A L   V A R I A B L E S:

       ncd         number of diagonals below/above main diagonal
       rcond       estimate of the reciprocal condition of matrix CBAND; for system  CBAND*X = B, relative
                   perturbations in CBAND and B of size epsilon may cause relative perturbations in X of size
                   epsilon/RCOND.  If RCOND is so small that
                          1.0 + RCOND .eq. 1.0
                   is true, then CBAND may be singular to working precision.
       cband       Left-hand side matrix of linear system eq. SC(5), scaled by eq. SC(12);
                   in banded form required by LINPACK solution routines
       ncol        number of columns in CBAND matrix
       ipvt        INTEGER vector of pivot indices (most others documented in DISORT)
  
   Called by- c_disort
   Calls- c_legendre_poly, c_sgbco, c_solve_eigen, c_interp_eigenvec, c_set_matrix, c_solve1,
          c_albtrans_intensity, c_albtrans_spherical, c_print_albtrans
 --------------------------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_albtrans(disort_state  *ds,
                disort_output *out,
                disort_pair   *ab,
                double        *array,
                double        *b,
                double        *bdr,
                double        *cband,
                double        *cc,
                double        *cmu,
                double        *cwt,
                double        *dtaucpr,
                double        *eval,
                double        *evecc,
                double        *gl,
                double        *gc,
                double        *gu,
                int           *ipvt,
                double        *kk,
                double        *ll,
                int            nn,
                double        *taucpr,
                double        *ylmc,
                double        *ylmu,
                double        *z,
                double        *wk)
{
  int
    lyrcut,ncol;
  int
    iq,iu,l,lc,mazim,ncd,ncut;
  double
    delm0,rcond,sgn,sphalb,sphtrn;

  mazim = 0;
  delm0 = 1.;
  /*
   * Set DISORT variables that are ignored in this special case but are needed below in argument
   * lists of subroutines shared with general case
   */
  ncut            = ds->nlyr;
  lyrcut          = FALSE;
  ds->bc.fisot    = 1.;
  ds->bc.fluor    = 0.;
  ds->flag.lamber = TRUE;

  /*
   * Get Legendre polynomials for computational and user polar angle cosines
   */
  c_legendre_poly(ds->numu,mazim,ds->nstr,ds->nstr-1,ds->umu,ylmu);
  c_legendre_poly(nn,      mazim,ds->nstr,ds->nstr-1,cmu,    ylmc);

  /*
   * Evaluate Legendre polynomials with negative arguments from those with positive arguments;
   * Dave/Armstrong eq. (15), STWL(59)
   */
  sgn = -1.0;
  for (l = mazim; l <= ds->nstr-1; l++) {
    sgn *= -1;
    for (iq = nn+1; iq <= ds->nstr; iq++) {
      YLMC(l,iq) = sgn*YLMC(l,iq-nn);
    }
  }

  /*
   * Zero out bottom reflectivity (ALBEDO is used only in analytic formulae involving ALBEDO = 0
   * solutions; eqs 16-17 of Ref S2)
   */
  memset(bdr,0,(ds->nstr/2)*((ds->nstr/2)+1)*sizeof(double));

  /*-------------------  BEGIN LOOP ON COMPUTATIONAL LAYERS  -------------*/
  for (lc = 1; lc <= ds->nlyr; lc++) {
    /*
     * Solve eigenfunction problem in eq. STWJ(8b), STWL(23f)
     */
    c_solve_eigen(ds,lc,ab,array,cmu,cwt,gl,mazim,nn,ylmc,cc,evecc,eval,kk,gc,wk);
    /*
     * Interpolate eigenvectors to user angles
     */
    c_interp_eigenvec(ds,lc,cwt,evecc,gl,gu,mazim,nn,wk,ylmc,ylmu);
  }
  /*------------------  END LOOP ON COMPUTATIONAL LAYERS  ---------------*/

  /*
   * Set coefficient matrix (CBAND) of equations
   * combining boundary and layer interface
   * conditions (in band-storage mode required by
   * LINPACK routines)
   */
  c_set_matrix(ds,bdr,cband,cmu,cwt,delm0,dtaucpr,gc,kk,lyrcut,&ncol,ncut,taucpr,wk);

  /*
   * LU-decompose the coeff. matrix (LINPACK)
   */
  ncd = 3*nn-1;
  c_sgbco(cband,(9*(ds->nstr/2)-2),ncol,ncd,ncd,ipvt,&rcond,z);
  if (1.+rcond == 1.) {
    c_errmsg("albtrans--sgbco says matrix near singular",DS_WARNING);
  }

  /*
   * First, illuminate from top; if only one layer, this will give us everything
   * Solve for constants of integration in homogeneous solution
   */
  c_solve1(ds,cband,TOP_ILLUM,ipvt,ncol,ncut,nn,b,ll);

  /*
   * Compute azimuthally-averaged intensity at user angles; gives albedo if multi-layer (eq. 9 of Ref S2);
   * gives both albedo and transmissivity if single layer (eqs. 3-4 of Ref S2)
   */
  c_albtrans_intensity(ds,out,gu,kk,ll,nn,taucpr,wk);

  /*
   * Get beam-incidence albedos from reciprocity principle
   */

  for (iu = 1; iu <= ds->numu/2; iu++) {
    ALBMED(iu) = U0U(iu+ds->numu/2,1);
  }
  if (ds->nlyr == 1) {
    for (iu = 1; iu <= ds->numu/2; iu++) {
      /*
       * Get beam-incidence transmissivities from reciprocity principle (1 layer);
       * flip them end over end to correspond to positive UMU instead of negative
       */
      TRNMED(iu) = U0U(ds->numu/2+1-iu,2)+exp(-TAUCPR(ds->nlyr)/UMU(iu+ds->numu/2));
    }
  }
  else {
    /*
     * Second, illuminate from bottom (if multiple layers)
     */
    c_solve1(ds,cband,BOT_ILLUM,ipvt,ncol,ncut,nn,b,ll);
    c_albtrans_intensity(ds,out,gu,kk,ll,nn,taucpr,wk);
    /*
     * Get beam-incidence transmissivities from reciprocity principle
     */
    for (iu = 1; iu <= ds->numu/2; iu++) {
      TRNMED(iu) = U0U(iu+ds->numu/2,1)+exp(-TAUCPR(ds->nlyr)/UMU(iu+ds->numu/2));
    }
  }

  if (ds->bc.albedo > 0.) {
    /*
     * Get spherical albedo and transmissivity
     */
    if (ds->nlyr == 1) {
      c_albtrans_spherical(ds,cmu,cwt,gc,kk,ll,nn,taucpr,&sphalb,&sphtrn);
    }
    else {
      c_albtrans_spherical(ds,cmu,cwt,gc,kk,ll,nn,taucpr,&sphtrn,&sphalb);
    }
    /*
     * Ref. S2, eqs. 16-17 (these eqs. have a simple physical interpretation
     * like that of adding-doubling eqs.)
     */
    for (iu = 1; iu <= ds->numu; iu++) {
      
      ALBMED(iu) += ds->bc.albedo/(1.-ds->bc.albedo*sphalb)*sphtrn*TRNMED(iu);
      TRNMED(iu) += ds->bc.albedo/(1.-ds->bc.albedo*sphalb)*sphalb*TRNMED(iu);
    }
  }
  /*
   * Return UMU to all positive values, to agree with ordering in ALBMED, TRNMED
   */
  ds->numu /= 2;
  for (iu = 1; iu <= ds->numu; iu++) {
    UMU(iu) = UMU(iu+ds->numu);
  }
  if (ds->flag.prnt[3]) {
    c_print_albtrans(ds,out);
  }
  
  /* CE: I want to output the the spherical albedo and transmittance, and use the */
  /* variables ALBMED and TRNMED for this. They are not used so far otherwise in uvspec */
  /* If somebody needs these variables I will include new variables for sphtrn and sphalb*/
  ALBMED(1)=sphalb;
  TRNMED(1)=sphtrn;
  
  return;
}

/*============================= end of c_albtrans() ======================*/

/*============================= c_albtrans_intensity() ===================*/

/*
   Computes azimuthally-averaged intensity at top and bottom of medium
   (related to albedo and transmission of medium by reciprocity principles;
   see Ref S2).  User polar angles are used as incident beam angles.
   (This is a very specializedversion of user_intensities)

   ** NOTE **  User input values of UMU (assumed positive) are temporarily in
               upper locations of  UMU  and corresponding negatives are in
               lower locations (this makes GU come out right); the contents
               of the temporary UMU array are:
                   -UMU(ds->numu),..., -UMU(1), UMU(1),..., UMU(ds->numu)

   I N P U T    V A R I A B L E S:

       ds     :  Disort state variables
       gu     :  Eigenvectors interpolated to user polar angles (i.e., g in eq. SC(1), STWL(31ab))
       kk     :  Eigenvalues of coeff. matrix in eq. SS(7), STWL(23b)
       ll     :  Constants of integration in eq. SC(1), obtained by solving scaled version of eq. SC(5);
                 exponential term of eq. SC(12) not included
       nn     :  Order of double-Gauss quadrature (NSTR/2)
       taucpr :  Cumulative optical depth (delta-M-scaled)

   O U T P U T    V A R I A B L E:

       out->u0u : Diffuse azimuthally-averaged intensity at top and bottom of medium (directly transmitted component,
                  corresponding to bndint in user_intensities, is omitted).

   I N T E R N A L    V A R I A B L E S:

       dtau   :  Optical depth of a computational layer
       palint :  Non-boundary-forced intensity component
       utaupr :  Optical depths of user output levels (delta-M scaled)
       wk     :  Scratch vector for saving 'EXP' evaluations
       All the exponential factors (i.e., exp1, expn,... etc.)
       come from the substitution of constants of integration in
       eq. SC(12) into eqs. S1(8-9).  All have negative arguments.

   Called by- c_albtrans
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_albtrans_intensity(disort_state *ds,
			  disort_output *out,
                          double       *gu,
                          double       *kk,
                          double       *ll,
                          int           nn,
                          double       *taucpr,
                          double       *wk)
{
  int
    iq,iu,iumax,iumin,lc,lu;
  double
    denom,dtau,exp1,exp2,expn,mu,palint,sgn,utaupr[2];

  UTAUPR(1) = 0.;
  UTAUPR(2) = TAUCPR(ds->nlyr);

  for (lu = 1; lu <= 2; lu++) {
    if (lu == 1) {
      iumin = ds->numu/2+1;
      iumax = ds->numu;
      sgn   = 1.;
    }
    else {
      iumin = 1;
      iumax = ds->numu/2;
      sgn   = -1.;
    }

    /*
     * Loop over polar angles at which albedos/transmissivities desired
     * ( upward angles at top boundary, downward angles at bottom )
     */
    for (iu = iumin; iu <= iumax; iu++) {
      mu = UMU(iu);
      /*
       * Integrate from top to bottom computational layer
       */
      palint = 0.;
      for (lc = 1; lc <= ds->nlyr; lc++) {
        dtau = TAUCPR(lc)-TAUCPR(lc-1);
        exp1 = exp((UTAUPR(lu)-TAUCPR(lc-1))/mu);
        exp2 = exp((UTAUPR(lu)-TAUCPR(lc  ))/mu);
        /*
         * KK is negative
         */
        for (iq = 1; iq <= nn; iq++) {
          WK(iq) = exp(KK(iq,lc)*dtau);
          denom  = 1.+mu*KK(iq,lc);
          if (fabs(denom) < 0.0001) {
            /*
             * L'Hospital limit
             */
            expn = dtau/mu*exp2;
          }
          else {
            expn = (exp1*WK(iq)-exp2)*sgn/denom;
          }
          palint += GU(iu,iq,lc)*LL(iq,lc)*expn;
        }
        /*
         * KK is positive
         */
        for (iq = nn+1; iq <= ds->nstr; iq++) {
          denom = 1.+mu*KK(iq,lc);
          if (fabs(denom) < 0.0001) {
            expn = -dtau/mu*exp1;
          }
          else {
            expn = (exp1-exp2*WK(ds->nstr+1-iq))*sgn/denom;
          }
          palint += GU(iu,iq,lc)*LL(iq,lc)*expn;
        }
      }
      U0U(iu,lu) = palint;
    }
  }

  return;
}

/*============================= end of c_albtrans_intensity() ============*/

/*============================= c_print_albtrans() =======================*/

/*
   Print planar albedo and transmissivity of medium as a function of
   incident beam angle

   Called by- c_albtrans
 --------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_print_albtrans(disort_state  *ds,
                      disort_output *out)
{
  int
    iu;

  fprintf(stdout,"\n\n\n *******  Flux Albedo and/or Transmissivity of entire medium  ********\n");
  fprintf(stdout," Beam Zen Ang   cos(Beam Zen Ang)      Albedo   Transmissivity\n");
  for (iu = 1; iu <= ds->numu; iu++) {
    fprintf(stdout,"%13.4f%20.6f%12.5f%17.4e\n",acos(UMU(iu))/DEG,UMU(iu),ALBMED(iu),TRNMED(iu));
  }

  return;
}

/*============================= end of c_print_albtrans() ================*/

/*============================= c_albtrans_spherical() ===================*/

/*
    Calculates spherical albedo and transmissivity for the entire medium
    from the m=0 intensity components (this is a specialized version of fluxes)

    I N P U T    V A R I A B L E S:

       ds      :  Disort state variables
       cmu,cwt :  Abscissae, weights for Gaussian quadrature over angle cosine
       kk      :  Eigenvalues of coeff. matrix in eq. SS(7)
       gc      :  Eigenvectors at polar quadrature angles, SC(1)
       ll      :  Constants of integration in eq. SC(1), obtained by solving
                  scaled version of eq. SC(5); exponential term of eq. SC(12) not incl.
       nn      :  Order of double-Gauss quadrature (NSTR/2)

    O U T P U T   V A R I A B L E S:

       sflup   :  Up-flux at top (equivalent to spherical albedo due to
                  reciprocity).  For illumination from below it gives
                  spherical transmissivity

       sfldn   :  Down-flux at bottom (for single layer, equivalent to
                  spherical transmissivity due to reciprocity)

    I N T E R N A L   V A R I A B L E S:

       zint    :  Intensity of m=0 case, in eq. SC(1)

   Called by- c_albtrans
 --------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_albtrans_spherical(disort_state *ds,
                          double       *cmu,
                          double       *cwt,
                          double       *gc,
                          double       *kk,
                          double       *ll,
                          int           nn,
                          double       *taucpr,
                          double       *sflup,
                          double       *sfldn)
{
  int
    iq,jq;
  double
    zint;

  *sflup = 0.;
  for (iq = nn+1; iq <= ds->nstr; iq++) {
    zint = 0.;
    for (jq = 1; jq <= nn; jq++) {
      zint += GC(iq,jq,1)*LL(jq,1)*exp(KK(jq,1)*TAUCPR(1));
    }
    for (jq = nn+1; jq <= ds->nstr; jq++) {
      zint += GC(iq,jq,1)*LL(jq,1);
    }
    *sflup += CWT(iq-nn)*CMU(iq-nn)*zint;
  }

  *sfldn = 0.;
  for (iq = 1; iq <= nn; iq++) {
    zint = 0.;
    for (jq = 1; jq <= nn; jq++) {
      zint += GC(iq,jq,ds->nlyr)*LL(jq,ds->nlyr);
    }
    for (jq = nn+1; jq <=ds->nstr; jq++) {
      zint += GC(iq,jq,ds->nlyr)*LL(jq,ds->nlyr)*exp(-KK(jq,ds->nlyr)*(TAUCPR(ds->nlyr)-TAUCPR(ds->nlyr-1)));
    }
    *sfldn += CWT(nn+1-iq)*CMU(nn+1-iq)*zint;
  }

  *sflup *= 2.;
  *sfldn *= 2.;

  return;
}

/*============================= end of c_albtrans_spherical() ============*/

/******************************************************************
 ********** End of ds->flag.ibcnd = SPECIAL_BC routines ***********
 ******************************************************************/

