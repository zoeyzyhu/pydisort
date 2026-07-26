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

/*============================= c_disort_set() ==========================*/

/*
    Perform miscellaneous setting-up operations

    I N P U T  V A R I A B L E S

       ds         Disort state variables
       deltam
       tauc

    O U T P U T     V A R I A B L E S:

       If ds->flag.usrtau is FALSE
       ds->ntau
       ds->utau 

       If ds->flag.usrang is FALSE
       ds->numu
       ds->umu
  
       cmu,cwt     computational polar angles and corresponding quadrature weights
       dtaucpr
       expbea      transmission of direct beam
       flyr        separated fraction in delta-m method
       gl          phase function legendre coefficients multiplied by (2l+1) and single-scatter albedo
       layru       computational layer in which utau falls
       lyrcut      flag as to whether radiation will be zeroed below layer ncut
       ncut        computational layer where absorption optical depth first exceeds  abscut
       nn          ds->nstr/2
       oprim       delta-m-scaled single-scatter albedo
       taucpr      delta-m-scaled optical depth
       utaupr      delta-m-scaled version of  utau

   Called by- c_disort
   Calls- c_gaussian_quadrature, c_errmsg

 ---------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_disort_set(disort_state *ds,
                  double       *ch,
                  double       *chtau,
                  double       *cmu,
                  double       *cwt,
                  int           deltam,
                  double       *dtaucpr,
                  double       *expbea,
                  double       *flyr,
                  double       *gl,
                  int          *layru,
                  int          *lyrcut,
                  int          *ncut,
                  int          *nn,
                  int          *corint,
                  double       *oprim,
                  double       *tauc,
                  double       *taucpr,
                  double       *utaupr,
                  emission_func_t emi_func)
{
  int
    iq,iu,k,lc,lu;
  const double
    abscut = 10.;
  double
    abstau,chtau_tmp,f,taup,zenang;

  if (!ds->flag.usrtau) {
   /*
    * Set output levels at computational layer boundaries
    */
    for (lc = 0;  lc <= ds->ntau-1; lc++) {
      UTAU(lc+1) = TAUC(lc);
    }
  }

  /*
   * Apply delta-M scaling and move description of computational layers to local variables
   */
  TAUCPR(0) = 0.;
  abstau    = 0.;
  for (lc = 1; lc <= ds->nlyr; lc++) {
    PMOM(0,lc)  = 1.;
    if (abstau < abscut) {
      *ncut = lc;
    }
    abstau += (1.-SSALB(lc))*DTAUC(lc);
    if (!deltam) {
      OPRIM(lc)   = SSALB(lc);
      DTAUCPR(lc) = DTAUC(lc);
      TAUCPR(lc)  = TAUC(lc);
      for (k = 0; k <= ds->nstr-1; k++) {
        GL(k,lc)  = (double)(2*k+1)*OPRIM(lc)*PMOM(k,lc);
      }
      f = 0.;
    }
    else {
      /*
       * Do delta-M transformation
       */
      f           = PMOM(ds->nstr,lc);
      OPRIM(lc)   = SSALB(lc)*(1.-f)/(1.-f*SSALB(lc));
      DTAUCPR(lc) = (1.-f*SSALB(lc))*DTAUC(lc);
      TAUCPR(lc)  = TAUCPR(lc-1)+DTAUCPR(lc);
      for (k = 0; k <= ds->nstr-1; k++) {
        GL(k,lc)  = (double)(2*k+1)*OPRIM(lc)*(PMOM(k,lc)-f)/(1.-f);
      }
    }

    FLYR(lc)   = f;
  }

  /*
   * Calculate Chapman function if spherical geometry, set expbea and
   * ch for beam source.
   */
  if( (ds->flag.ibcnd == GENERAL_BC && ds->bc.fbeam > 0.) ||
      (ds->flag.ibcnd == GENERAL_BC && ds->flag.general_source )) {

    CHTAU(0)  = 0.;
    EXPBEA(0) = 1.;
    zenang    = acos(ds->bc.umu0)/DEG;
    
    if( ds->flag.spher == TRUE && ds->bc.umu0 < 0. ) {
      EXPBEA(0) = exp(-c_chapman(1,0.,tauc,ds->nlyr,ds->zd,
				 ds->dtauc,zenang,ds->radius));
    }
    if ( ds->flag.spher == TRUE ) {
      for (lc = 1; lc <= *ncut; lc++) {
        taup        = TAUCPR(lc-1) + DTAUCPR(lc)/2.;
	/* Need Chapman function at top (0.0) and middle (0.5) of layer */
        CHTAU(lc  ) = c_chapman(lc, 0.,   taucpr,ds->nlyr,ds->zd,
				dtaucpr,zenang,ds->radius);
        chtau_tmp   = c_chapman(lc, 0.5,  taucpr,ds->nlyr,ds->zd,
				dtaucpr,zenang,ds->radius);
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
  else {
    for (lc = 1; lc <= *ncut; lc++) {
      EXPBEA(lc) = 0.;
    }
  }

  /*
   * If no thermal emission, cut off medium below absorption optical depth = abscut ( note that
   * delta-M transformation leaves absorption optical depth invariant ).  Not worth the
   * trouble for one-layer problems, though.
   */
  *lyrcut = FALSE;
  if (abstau >= abscut && !ds->flag.planck && ds->flag.ibcnd != SPECIAL_BC && ds->nlyr > 1) {
    *lyrcut = TRUE;
  }
  if(!*lyrcut) *ncut = ds->nlyr;

  /* 
   * Set arrays defining location of user output levels within delta-M-scaled computational mesh
   */
  for (lu = 1; lu <= ds->ntau; lu++) {
    for (lc = 1; lc < ds->nlyr; lc++) {
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
   * Calculate computational polar angle cosines and associated quadrature weights for Gaussian
   * quadrature on the interval (0,1) (upward)
   */
  *nn = ds->nstr/2;
  c_gaussian_quadrature(*nn,cmu,cwt);

  /*
   * Downward (neg) angles and weights
   */
  for (iq = 1; iq <= *nn; iq++) {
    CMU(iq+*nn) = -CMU(iq);
    CWT(iq+*nn) =  CWT(iq);
  }

  if (ds->flag.ibcnd == GENERAL_BC && ds->bc.fbeam > 0.) {
    /*
     * Compare beam angle to comput. angles
     */
    for (iq = 1; iq <= *nn; iq++) {      
      if (fabs(ds->bc.umu0-CMU(iq))/fabs(ds->bc.umu0) < 1.e-4) {
        // suppress error msg by adding a small difference
        ds->bc.umu0 = (1. + 1.E-4)*CMU(iq);
        // c_errmsg("cdisort_set--beam angle=computational angle; change ds.nstr",DS_ERROR);
      }
    }
  }

  if (!ds->flag.usrang || ds->flag.onlyfl) {
    /*
     * Set output polar angles to computational polar angles
     */
    for (iu = 1; iu <= *nn; iu++) {
      UMU(iu) = -CMU(*nn+1-iu);
    }
    for (iu = *nn+1; iu <=ds->nstr; iu++) {
      UMU(iu) =  CMU(iu-*nn);
    }
  }

  if (ds->flag.usrang && ds->flag.ibcnd == SPECIAL_BC) {
    /*
     * Shift positive user angle cosines to upper locations and put negatives in lower locations
     */
    for (iu = 1; iu <= ds->numu/2; iu++) {
      UMU(iu+ds->numu/2) = UMU(iu);
    }
    for (iu = 1; iu <= ds->numu/2; iu++) {
      UMU(iu) = -UMU((ds->numu/2)+1-iu);
    }
  }

  return;
}

/*============================= end of c_disort_set() ===================*/

/*============================= c_set_matrix() ==========================*/

/*
    Calculate coefficient matrix for the set of equations obtained from the
    boundary conditions and the continuity-of-intensity-at-layer-interface equations.

    Store in the special banded-matrix format required by LINPACK routines


    I N P U T      V A R I A B L E S:

       ds       :  Disort state variables
       bdr      :  surface bidirectional reflectivity
       cmu,cwt  :  abscissae, weights for Gauss quadrature over angle cosine
       delm0    :  Kronecker delta, delta-sub-m0
       gc       :  Eigenvectors at polar quadrature angles, SC(1)
       kk       :  Eigenvalues of coeff. matrix in eq. SS(7), STWL(23b)
       lyrcut   :  Logical flag for truncation of computational layers
       nn       :  Number of streams in a hemisphere (NSTR/2)
       ncut     :  Total number of computational layers considered
       taucpr   :  Cumulative optical depth (delta-M-scaled)

   O U T P U T     V A R I A B L E S:

       cband    :  Left-hand side matrix of linear system eq. SC(5), scaled by eq. SC(12); 
                   in banded form required by LINPACK solution routines
       ncol     :  Number of columns in cband


   I N T E R N A L    V A R I A B L E S:

       irow     :  Points to row in CBAND
       jcol     :  Points to position in layer block
       lda      :  Row dimension of CBAND
       ncd      :  Number of diagonals below or above main diagonal
       nshift   :  For positioning number of rows in band storage
       wk       :  Temporary storage for EXP evaluations


   BAND STORAGE

      LINPACK requires band matrices to be input in a special
      form where the elements of each diagonal are moved up or
      down (in their column) so that each diagonal becomes a row.
      (The column locations of diagonal elements are unchanged.)

      Example:  if the original matrix is

          11 12 13  0  0  0
          21 22 23 24  0  0
           0 32 33 34 35  0
           0  0 43 44 45 46
           0  0  0 54 55 56
           0  0  0  0 65 66

      then its LINPACK input form would be:

           *  *  *  +  +  +  , * = not used
           *  * 13 24 35 46  , + = used for pivoting
           * 12 23 34 45 56
          11 22 33 44 55 66
          21 32 43 54 65  *

      If A is a band matrix, the following program segment
      will convert it to the form (ABD) required by LINPACK
      band-matrix routines:

        n  = (column dimension of a, abd)
        ml = (band width below the diagonal)
        mu = (band width above the diagonal)
        m = ml+mu+1;
        for (j = 1; j <= n; j++) {
          i1 = IMAX(1,j-mu);
          i2 = IMIN(n,j+ml);
          for (i = i1; i <= i2; i++) {
            k = i-j+m;
            ABD(k,j) = A(i,j);
          }
        }

      This uses rows  ml+1 through  2*ml+mu+1  of ABD.
      The total number of rows needed in ABD is 2*ml+mu+1.
      In the example above, n = 6, ml = 1, mu = 2, and the
      row dimension of ABD must be >= 5.

   Called by- c_disort, c_albtrans
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_set_matrix(disort_state *ds,
                  double       *bdr,
                  double       *cband,
                  double       *cmu,
                  double       *cwt,
                  double        delm0,
                  double       *dtaucpr,
                  double       *gc,
                  double       *kk,
                  int           lyrcut,
                  int          *ncol,
                  int           ncut,
                  double       *taucpr,
                  double       *wk)
{
  int
    mi     = ds->nstr/2,
    mi9m2  = 9*mi-2,
    nnlyri = ds->nstr*ds->nlyr,
    nn     = ds->nstr/2;
  int
    iq,irow,jcol,jq,k,lc,lda,ncd,nncol,nshift;
  double
    expa,sum;

  memset(cband,0,mi9m2*nnlyri*sizeof(double));

  ncd    = 3*nn-1;
  lda    = 3*ncd+1;
  nshift = lda-2*ds->nstr+1;
  *ncol  = 0;

  /*
   * Use continuity conditions of eq. STWJ(17) to form coefficient matrix in STWJ(20);
   * employ scaling transformation STWJ(22)
   */
  for (lc = 1; lc <= ncut; lc++) {
    for (iq = 1; iq <= nn; iq++) {
      WK(iq) = exp(KK(iq,lc)*DTAUCPR(lc));
    }
    jcol = 0;
    for (iq = 1; iq <= nn; iq++) {
      *ncol += 1;
      irow   = nshift-jcol;
      for (jq = 1; jq <= ds->nstr; jq++) {
        CBAND(irow+ds->nstr,*ncol) =  GC(jq,iq,lc);
        CBAND(irow,         *ncol) = -GC(jq,iq,lc)*WK(iq);
        irow++;
      }
      jcol++;
    }

    for (iq = nn+1; iq <= ds->nstr; iq++) {
      *ncol += 1;
      irow = nshift-jcol;
      for (jq = 1; jq <= ds->nstr; jq++) {
        CBAND(irow+ds->nstr,*ncol) =  GC(jq,iq,lc)*WK(ds->nstr+1-iq);
        CBAND(irow,         *ncol) = -GC(jq,iq,lc);
        irow++;
      }
      jcol++;
    }
  }

  /*
   * Use top boundary condition of STWJ(20a) for first layer
   */
  jcol = 0;
  for (iq = 1; iq <= nn; iq++) {
    expa = exp(KK(iq,1)*TAUCPR(1));
    irow = nshift-jcol+nn;
    for (jq = nn; jq >= 1; jq--) {
      CBAND(irow,jcol+1) = GC(jq,iq,1)*expa;
      irow++;
    }
    jcol++;
  }

  for (iq = nn+1; iq <=ds->nstr; iq++) {
    irow = nshift-jcol+nn;
    for (jq = nn; jq >= 1; jq--) {
      CBAND(irow,jcol+1) = GC(jq,iq,1);
      irow++;
    }
    jcol++;
  }

  /*
   * Use bottom boundary condition of STWJ(20c) for last layer
   */
  nncol = *ncol-ds->nstr;
  jcol  = 0;
  for (iq = 1; iq <= nn; iq++) {
    nncol++;
    irow = nshift-jcol+ds->nstr;
    for (jq = nn+1; jq <= ds->nstr; jq++) {
      if (lyrcut || ( ds->flag.lamber && delm0 == 0. ) ) {
        /*
         * No azimuthal-dependent intensity if Lambert surface; 
         * no intensity component if truncated bottom layer
         */
        CBAND(irow,nncol) = GC(jq,iq,ncut);
      }
      else {
        sum = 0.;
        for (k = 1; k <= nn; k++) {
          sum += CWT(k)*CMU(k)*BDR(jq-nn,k)*GC(nn+1-k,iq,ncut);
        }
        CBAND(irow,nncol) = GC(jq,iq,ncut)-(1.+delm0)*sum;
      }
      irow++;
    }
    jcol++;
  }

  for (iq = nn+1; iq <= ds->nstr; iq++) {
    nncol++;
    irow = nshift-jcol+ds->nstr;
    expa = WK(ds->nstr+1-iq);
    for (jq = nn+1; jq <= ds->nstr; jq++) {
      if (lyrcut || (ds->flag.lamber && delm0 == 0.)) {
        CBAND(irow,nncol) = GC(jq,iq,ncut)*expa;
      }
      else {
        sum = 0.;
        for (k = 1; k <= nn; k++) {
          sum += CWT(k)*CMU(k)*BDR(jq-nn,k)*GC(nn+1-k,iq,ncut);
        }
        CBAND(irow,nncol) = (GC(jq,iq,ncut)-(1.+delm0)*sum)*expa;
      }
      irow++;
    }
    jcol++;
  }

  return;
}

/*============================= end of c_set_matrix() ===================*/

/*============================= c_check_inputs() ========================*/

/*
 * Checks the input dimensions and variables
 *
 * Calls- c_write_bad_var, c_dref, c_errmsg
 * Called by- c_disort
 */

DISPATCH_MACRO inline int c_check_inputs(disort_state *ds,
		    int           scat_yes,
		    int           deltam,
		    int           corint,
		    double       *tauc,
		    int           callnum)
{
  int
    inperr = FALSE;
  int
    irmu,iu,j,k,lc,lu, nu;
  double
    flxalb,rmu,umumin;

  if (ds->nstr < 2 || ds->nstr%2 != 0) {
    inperr = c_write_bad_var(VERBOSE,"ds.nstr");
  }
  if (ds->nstr == 2) {
    c_errmsg("check_inputs()--2 streams not recommended;\n\nUse specialized 2-stream code c_twostr() instead",DS_WARNING);
  }
  if (ds->nlyr < 1) {
    inperr = c_write_bad_var(VERBOSE,"ds.nlyr");
  }

  for (lc = 1; lc <= ds->nlyr; lc++) {
    if (DTAUC(lc) < 0.) {
      inperr = c_write_bad_var(VERBOSE,"ds.dtauc");
    }
    if (SSALB(lc) < 0.0 || SSALB(lc) > 1.0) {
      inperr = c_write_bad_var(VERBOSE,"ds.ssalb");
    }
    if (ds->flag.ibcnd == GENERAL_BC) {
      if (ds->flag.planck) {
        if (lc == 1 && TEMPER(0) < 0.) {
          inperr = c_write_bad_var(VERBOSE,"ds.temper");
        }
        if (TEMPER(lc) < 0.) {
          inperr = c_write_bad_var(VERBOSE,"ds.temper");
        }
      }
    }
    else if (ds->flag.ibcnd == SPECIAL_BC) {
      ds->flag.planck = FALSE;
    }
    else {
      c_errmsg("check_inputs---unrecognized ds->flag.ibcnd",DS_ERROR);
    }
  }

  if (ds->nmom < 0 || (scat_yes  && ds->nmom < ds->nstr)) {
    inperr = c_write_bad_var(VERBOSE,"ds.nmom");
  }

  for (lc = 1; lc <= ds->nlyr; lc++) {
    for (k = 0; k <= ds->nmom; k++) {
      if (PMOM(k,lc) < -1. || PMOM(k,lc) > 1.) {
        inperr = c_write_bad_var(VERBOSE,"PMOM(k,lc)");
      }
    }
  }

  if( ds->flag.spher == TRUE ) {
    for (lc = 1; lc <= ds->nlyr; lc++) {
      if (ds->ZD(lc) > ds->ZD(lc-1)) {
        inperr     = c_write_bad_var(ds->flag.quiet,"zd");
      }
    }
  }
 
  if (ds->flag.ibcnd == GENERAL_BC) {
    if (ds->flag.usrtau) {
      if (ds->ntau < 1) {
        inperr = c_write_bad_var(VERBOSE,"ds.ntau");
      }
      for (lu = 1; lu <= ds->ntau; lu++) {
	/* Do a relative check to see if we are just beyond the bottom boundary */
	/* This might happen due to numerical rounding off problems.  ak20110224*/
        if (fabs(UTAU(lu)-TAUC(ds->nlyr)) <= 1.e-6*TAUC(ds->nlyr)) {
          UTAU(lu) = TAUC(ds->nlyr);
        }
        if(UTAU(lu) < 0. || UTAU(lu) > TAUC(ds->nlyr)) {
          inperr = c_write_bad_var(VERBOSE,"ds.utau");
        }
      }
    }
  }

  if (ds->flag.usrang) {
    if (ds->numu < 0) {
      inperr = c_write_bad_var(VERBOSE,"ds.numu");
    }
    if (!ds->flag.onlyfl && ds->numu == 0) {
      inperr = c_write_bad_var(VERBOSE,"ds.numu");
    }
    nu = ds->numu;
    if (ds->flag.ibcnd == SPECIAL_BC ) nu = ds->numu/2;
    for (iu = 1; iu <= nu; iu++) {
      if (UMU(iu) < -1. || UMU(iu) > 1. || UMU(iu) == 0.) {
        inperr = c_write_bad_var(VERBOSE,"ds.umu");
      }
      if (ds->flag.ibcnd == SPECIAL_BC && UMU(iu) < 0.) {
        inperr = c_write_bad_var(VERBOSE,"ds.umu");
      }
      if (iu > 1) {
        if (UMU(iu) < UMU(iu-1)) {
          inperr = c_write_bad_var(VERBOSE,"ds.umu");
        }
      }
    }
  }

  if (!ds->flag.onlyfl && ds->flag.ibcnd != SPECIAL_BC) {
    if (ds->nphi <= 0) {
      inperr = c_write_bad_var(VERBOSE,"ds.nphi");
    }
    for (j=1; j <=ds->nphi; j++) {
      if (PHI(j) < 0. || PHI(j) > 360.) {
        inperr = c_write_bad_var(VERBOSE,"ds.phi");
      }
    }
  }

  if (ds->flag.ibcnd != GENERAL_BC && ds->flag.ibcnd != SPECIAL_BC) {
    inperr = c_write_bad_var(VERBOSE,"ds.flag.ibcnd");
  }

  if (ds->flag.ibcnd == GENERAL_BC) {
    if (ds->bc.fbeam < 0.) {
      inperr = c_write_bad_var(VERBOSE,"ds.bc.fbeam");
    }
    else if (ds->bc.fbeam > 0.) {
      umumin = 0.;
      if( ds->flag.spher == TRUE ) {
	umumin = -1.;
      }
      if (ds->bc.umu0 <= umumin || ds->bc.umu0 > 1.) {
        inperr = c_write_bad_var(VERBOSE,"ds.bc.umu0");
      }
      if (ds->bc.phi0 < 0. || ds->bc.phi0 > 360.) {
        inperr = c_write_bad_var(VERBOSE,"ds.bc.phi0");
      }
    }

    if (ds->bc.fisot < 0.) {
      inperr = c_write_bad_var(VERBOSE,"ds.bc.fisot");
    }

    if (ds->flag.lamber) {
      if (ds->bc.albedo < 0. || ds->bc.albedo > 1.) {
        inperr = c_write_bad_var(VERBOSE,"ds.bc.albedo");
      }
    }
    else {
      /*
       * Make sure flux albedo at dense mesh of incident angles does not assume unphysical values
       */
      for (irmu = 0; irmu <= 100; irmu++) {
        rmu    = (double)irmu*0.01;
        flxalb = c_dref(ds->wvnmlo, ds->wvnmhi, rmu, ds->flag.brdf_type, &ds->brdf, callnum);
        if (flxalb < 0. || flxalb > 1.) {
          inperr = c_write_bad_var(VERBOSE,"bidir_reflectivity()");
        }
      }
    }
  }
  else {
    if (ds->bc.albedo < 0. || ds->bc.albedo > 1.) {
      inperr = c_write_bad_var(VERBOSE,"ds.bc.albedo");
    }
  }

  if (ds->flag.planck && ds->flag.ibcnd != SPECIAL_BC) {
    if (ds->wvnmlo < 0. || ds->wvnmhi < ds->wvnmlo) {
      inperr = c_write_bad_var(VERBOSE,"ds.wvnmlo,hi");
    }
    if (ds->bc.temis < 0. || ds->bc.temis > 1.) {
      inperr = c_write_bad_var(VERBOSE,"ds.bc.temis");
    }
    if (ds->bc.btemp < 0.) {
      inperr = c_write_bad_var(VERBOSE,"ds.bc.btemp");
    }
    if (ds->bc.ttemp < 0.) {
      inperr = c_write_bad_var(VERBOSE,"ds.bc.ttemp");
    }
  }

  if (ds->accur < 0. || ds->accur > 1.e-2) {
    inperr = c_write_bad_var(VERBOSE,"ds.accur");
  }

  if (inperr) {
    c_errmsg("DISORT--input and/or dimension errors",DS_WARNING);
    return 1;
  }

  if (ds->flag.planck && ds->flag.quiet == VERBOSE) {
    for (lc = 1; lc <= ds->nlyr; lc++) {
      if (fabs(TEMPER(lc)-TEMPER(lc-1)) > 10.) {
        c_errmsg("check_inputs--vertical temperature step may be too large for good accuracy",DS_WARNING);
      }
    }
  }
  if(!corint && (!ds->flag.onlyfl && ds->bc.fbeam > 0. && scat_yes && deltam)) {
    c_errmsg("check_inputs--intensity correction is off;\nintensities may be less accurate",DS_WARNING);
  }

  return 0;
}

/*============================= end of c_check_inputs() =================*/

/*============================= c_setout() ==============================*/

/*-------------------------------------------------------------------
 * Copyright (C) 1994 Arve Kylling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * To obtain a copy of the GNU General Public License write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *-------------------------------------------------------------------
 * Linearly interpolate to get approximate tau corresponding to 
 * altitude zout
 *
 * Input/output variables described in phodis.f
 *
 *
 * This code was translated to c from fortran by Robert Buras
 *
 */

DISPATCH_MACRO inline int c_setout( float *sdtauc,
	      int    nlyr,
	      int    ntau,
	      float *sutau,
	      float *z,
	      float *zout )
{
  int itau=0, lc=0, itype=0;
  double hh=0.0;
  double *tauint=NULL;

  tauint = c_dbl_vector(0,nlyr+1,"tauint");

  if (tauint==NULL) {
    fprintf(stderr,"Error allocating tauint!\n");
    return -1;
  }

  /* */     

  TAUINT (1) = 0.0;
  for (lc=1; lc<=nlyr; lc++)
    TAUINT (lc+1) = TAUINT (lc) + SDTAUC (lc);

  itype = 2;

  for (itau=1; itau<=ntau; itau++) 
    SUTAU (itau) = c_inter( nlyr+1, itype, ZOUT (itau),
			    z, tauint, &hh );

  pfree(tauint);

  return 0;
}

/*============================= end of c_setout() =======================*/

/*============================= c_inter() ===============================*/

/*-------------------------------------------------------------------
 * Copyright (C) 1994 Arve Kylling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * To obtain a copy of the GNU General Public License write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *-------------------------------------------------------------------
 *
 *     Interpolates at the x-point arg from x-value array xarr and
 *     y-value array yarr. xarr and yarr are expected to have
 *     descending arguments, i.e. for atmospheric applications
 *     xarr typically holds the altitude and inter expects
 *     xarr(1) = altitude at top of atmosphere.
 *
 *     Input variables:
 *     dim       Array dimension of xarr and yarr
 *     npoints   No. points in arrays xarr and yarr
 *     itype     Interpolation type
 *     arg       Interpolation argument
 *     xarr      array of x values
 *     yarr      array of y values
 *
 *     Output variables:
 *     ynew      Interpolated function value at arg
 *     hh        gradient or scale height value  
 *
 * This code was translated to c from fortran by Robert Buras
 *
 */

DISPATCH_MACRO inline double c_inter( int     npoints,
		int     itype,
		double  arg,
		float  *xarr,
		double *yarr,
		double *hh )
{
  int iq=0, ip=0;
  double ynew=0.0;

  if ( arg <= XARR (1) && arg >= XARR (npoints) ) {
    for (iq=1;iq<=npoints-1;iq++)
      if ( arg <= XARR (iq) && arg >= XARR (iq+1) )
	ip=iq;
    if ( arg == XARR (npoints) )
      ip = npoints - 1;
  }
  else {
    if ( arg > XARR (1) )
      ip = 1;
    else {
      if ( arg < XARR (npoints) )
	ip = npoints - 1;
    }
  }

  /* Interpolate function value at arg from data points ip to ip+1 */

  switch(itype) {
  case 1:
    /*     exponential interpolation */
    if ( YARR (ip+1) == YARR (ip) ) {
      *hh = 0.0;
      ynew = YARR (ip);
    }
    else {
      *hh = -( XARR (ip+1) - XARR (ip) ) / 
	log( YARR (ip+1) / YARR (ip));
      ynew = YARR (ip) * exp(- ( arg - XARR (ip) ) / *hh );
    }
    break;
  case 2:
    /*     linear interpolation */
    *hh = ( YARR (ip+1) - YARR (ip) ) / ( XARR (ip+1) - XARR (ip) );
    ynew = YARR (ip) + *hh * ( arg - XARR (ip) );
    break;
  default:
    fprintf (stderr, "Error, unknown itype %d (line %d, function '%s' in '%s')\n",
	     itype, __LINE__, __func__, __FILE__);
    return -999.0;
  }

  return ynew;
}

/*============================= end of c_inter() ========================*/

