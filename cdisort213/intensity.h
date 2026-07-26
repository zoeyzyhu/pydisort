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

#include "locate.h"

/*============================= c_intensity_components() ================*/

/*
    Calculates the Fourier intensity components at the quadrature
    angles for azimuthal expansion terms (mazim) in eq. SD(2),STWL(6)

    I N P U T    V A R I A B L E S:

       ds      :  Disort state variables
       kk      :  Eigenvalues of coeff. matrix in eq. SS(7), STWL(23b)
       gc      :  Eigenvectors at polar quadrature angles in eq. SC(1)
       ll      :  Constants of integration in eq. SC(1), obtained by solving scaled version of eq. SC(5);
                  exponential term of eq. SC(12) not included
       lyrcut  :  Logical flag for truncation of computational layer
       mazim   :  Order of azimuthal component
       ncut    :  Number of computational layer where absorption optical depth exceeds ABSCUT
       nn      :  Order of double-Gauss quadrature (NSTR/2)
       taucpr  :  Cumulative optical depth (delta-M-scaled)
       utaupr  :  Optical depths of user output levels in delta-M coordinates;  equal to UTAU if no delta-M
       zz      :  Beam source vectors in eq. SS(19), STWL(24b)
       plk     :  Thermal source vectors z0,z1 by solving eq. SS(16), Y-sub-zero, Y-sub-one in STWL(26ab);
                  plk[].zero, plk[].one (see cdisort.h)

    O U T P U T   V A R I A B L E S:

       uum     :  Fourier components of the intensity in eq. SD(12) (at polar quadrature angles)

    I N T E R N A L   V A R I A B L E S:

       fact    :  exp(-utaupr/umu0)
       zint    :  intensity of m=0 case, in eq. SC(1)

   Called by- c_disort
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_intensity_components(disort_state *ds,
                            double       *gc,
                            double       *kk,
                            int          *layru,
                            double       *ll,
                            int           lyrcut,
                            int           mazim,
                            int           ncut,
                            int           nn,
                            double       *taucpr,
                            double       *utaupr,
                            double       *zz,
                            disort_pair  *plk,
                            double       *uum)
{
  int
    iq,jq,lu,lyu;
  double
    zint;

  /*
   * Loop over user levels
   */
  for (lu = 1; lu <= ds->ntau; lu++) {
    lyu = LAYRU(lu);
    if (lyrcut && lyu > ncut) {
      continue;
    }
    for (iq = 1; iq <= ds->nstr; iq++) {
      zint = 0.;
      for (jq = 1; jq <= nn; jq++) {
        zint += GC(iq,jq,lyu)*LL(jq,lyu)*exp(-KK(jq,lyu)*(UTAUPR(lu)-TAUCPR(lyu  )));
      }
      for (jq = nn+1; jq <=ds->nstr; jq++) {
        zint += GC(iq,jq,lyu)*LL(jq,lyu)*exp(-KK(jq,lyu)*(UTAUPR(lu)-TAUCPR(lyu-1)));
      }
      UUM(iq,lu) = zint;
      if (ds->bc.fbeam > 0.) {
        UUM(iq,lu) = zint+ZZ(iq,lyu)*exp(-UTAUPR(lu)/ds->bc.umu0);
      }
      if (ds->flag.planck && mazim == 0) {
        UUM(iq,lu) += ZPLK0(iq,lyu)+ZPLK1(iq,lyu)*UTAUPR(lu);
      }
    }
  }

  return;
}

/*============================= end of c_intensity_components() =========*/

/*============================= c_intensity_correction() ================*/

/*
       Corrects intensity field by using Nakajima-Tanaka algorithm
       (1988). For more details, see Section 3.6 of STWL NASA report.
                I N P U T   V A R I A B L E S

       ds      Disort state variables
       dither  small multiple of machine precision
       flyr    separated fraction in delta-M method
       layru   index of UTAU in multi-layered system
       lyrcut  logical flag for truncation of computational layer
       ncut    total number of computational layers considered
       oprim   delta-M-scaled single-scatter albedo
       phirad  azimuthal angles in radians
       tauc    optical thickness at computational levels
       taucpr  delta-M-scaled optical thickness
       utaupr  delta-M-scaled version of UTAU

                O U T P U T   V A R I A B L E S

       out->UU  corrected intensity field; UU(IU,LU,J)
                 iu=1,ds->numu; lu=1,ds->ntau; j=1,ds->nphi

                I N T E R N A L   V A R I A B L E S

       ctheta  cosine of scattering angle
       dtheta  angle (degrees) to define aureole region as
                    direction of beam source +/- DTHETA
       phasa   actual (exact) phase function
       phasm   delta-M-scaled phase function
       phast   phase function used in TMS correction; actual phase
                    function divided by (1-FLYR*SSALB)
       pl      ordinary Legendre polynomial of degree l, P-sub-l
       plm1    ordinary Legendre polynomial of degree l-1, P-sub-(l-1)
       plm2    ordinary Legendre polynomial of degree l-2, P-sub-(l-2)
       theta0  incident zenith angle (degrees)
       thetap  emergent angle (degrees)
       ussndm  single-scattered intensity computed by using exact
                   phase function and scaled optical depth
                   (first term in STWL(68a))
       ussp    single-scattered intensity from delta-M method
                   (second term in STWL(68a))
       duims   intensity correction term from IMS method
                   (delta-I-sub-IMS in STWL(A.19))

   Called by- c_disort
   Calls- c_single_scat, c_secondary_scat
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_intensity_correction(disort_state  *ds,
                            disort_output *out,
                            double         dither,
                            double        *flyr,
                            int           *layru,
                            int            lyrcut,
                            int            ncut,
                            double        *oprim,
                            double        *phasa,
                            double        *phast,
                            double        *phasm,
                            double        *phirad,
                            double        *tauc,
                            double        *taucpr,
                            double        *utaupr)
{
  int
    iu,jp,k,lc,ltau,lu;
  double
    ctheta,dtheta,duims,pl,plm1,plm2,
    theta0=0,thetap=0,ussndm,ussp;

  dtheta = 10.;

  /*
   * Start loop over zenith angles
   */
  for (iu = 1; iu <= ds->numu; iu++) {
    if (UMU(iu) < 0.) {
      /*
       * Calculate zenith angles of incident and emerging directions
       */
      theta0 = acos(-ds->bc.umu0)/DEG;
      thetap = acos(UMU(iu))/DEG;
    }
    /*
     * Start loop over azimuth angles
     */
    for (jp = 1; jp <= ds->nphi; jp++) {
      /*
       * Calculate cosine of scattering angle, eq. STWL(4)
       */
      ctheta = -ds->bc.umu0*UMU(iu)+sqrt((1.-SQR(ds->bc.umu0))*(1.-SQR(UMU(iu))))*cos(PHIRAD(jp));
       /*
        * Initialize phase function                              
        */
      for (lc = 1; lc <= ncut; lc++) {
        PHASA(lc) = 1.;
        PHASM(lc) = 1.;
      }
      /*
       * Initialize Legendre poly. recurrence
       */

      plm1 = 1.;
      plm2 = 0.;
      for (k = 1; k <= ds->nmom; k++) {
        /*
         * Calculate Legendre polynomial of P-sub-l by upward recurrence
         */
        pl   = ((double)(2*k-1)*ctheta*plm1-(double)(k-1)*plm2)/k;
        plm2 = plm1;
        plm1 = pl;

        /*
         * Calculate actual phase function
         */
        for (lc = 1; lc <= ncut; lc++) {
          PHASA(lc) += (double)(2*k+1)*pl*PMOM(k,lc);
        }
        /*
         * Calculate delta-M transformed phase function
         */
        if (k <= ds->nstr-1) {
          for (lc = 1; lc <= ncut; lc++) {
            PHASM(lc) += (double)(2*k+1)*pl*(PMOM(k,lc)-FLYR(lc))/(1.-FLYR(lc));
          }
        }
      }
      /*
       * Apply TMS method, eq. STWL(68)
       */
      for (lc = 1; lc <= ncut; lc++) {
        PHAST(lc) = PHASA(lc)/(1.-FLYR(lc)*SSALB(lc));
      }
      for (lu = 1; lu <= ds->ntau; lu++) {
        if (!lyrcut || LAYRU(lu) < ncut) {
          ussndm        = c_single_scat(dither,LAYRU(lu),ncut,phast,ds->ssalb,taucpr,UMU(iu),ds->bc.umu0,UTAUPR(lu),ds->bc.fbeam);
          ussp          = c_single_scat(dither,LAYRU(lu),ncut,phasm,oprim,    taucpr,UMU(iu),ds->bc.umu0,UTAUPR(lu),ds->bc.fbeam);
          UU(iu,lu,jp) += ussndm-ussp;
        }
      }
      if (UMU(iu) < 0. && fabs(theta0-thetap) <= dtheta) {
        /*
         * Emerging direction is in the aureole (theta0 +/- dtheta).
         * Apply IMS method for correction of secondary scattering below top level.
         */
        ltau = 1;
        if (UTAU(1) <= dither) {
          ltau = 2;
        }
        for (lu = ltau; lu <= ds->ntau; lu++) {
          if(!lyrcut || LAYRU(lu) < ncut) {
            duims         = c_secondary_scat(ds,iu,lu,ctheta,flyr,LAYRU(lu),tauc);
	    UU(iu,lu,jp) -= duims;
          }
        }
      } 
    } /* end loop over azimuth angles */
  } /* end loop over zenith angles */

  return;
}

/*============================= end of c_intensity_correction() =========*/

/*============================= c_secondary_scat() ======================*/

/*
   Calculates secondary scattered intensity of eq. STWL (A7)

                I N P U T   V A R I A B L E S

        ds      Disort state variables
        iu      index of user polar angle
        lu      index of user level
        ctheta  cosine of scattering angle
        flyr    separated fraction f in Delta-M method
        layru   index of utau in multi-layered system
        tauc    cumulative optical depth at computational layers

                I N T E R N A L   V A R I A B L E S

        pspike  2*p"-p"*p", where p" is the residual phase function
        wbar    mean value of single scattering albedo
        fbar    mean value of separated fraction f
        dtau    layer optical depth
        stau    sum of layer optical depths between top of atmopshere and layer layru

   Called by- c_intensity_correction
   Calls- c_xi_func
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline double c_secondary_scat(disort_state *ds,
                        int           iu,
                        int           lu,
                        double        ctheta,
                        double       *flyr,
                        int           layru,
                        double       *tauc)
{
  int
    k,lyr;
  const double
    tiny = 1.e-4;
  double
    dtau,fbar,gbar,pl,plm1,plm2,pspike,
    stau,umu0p,wbar;
  double
    tmp;

  /*
   * Calculate vertically averaged value of single scattering albedo and separated
   * fraction f, eq. STWL (A.15)
   */
  dtau = UTAU(lu)-TAUC(layru-1);
  wbar = SSALB(layru)*dtau;
  fbar = FLYR(layru)*wbar;
  stau = dtau;
  for (lyr = 1; lyr <= layru-1; lyr++) {
    wbar += DTAUC(lyr)*SSALB(lyr);
    fbar += DTAUC(lyr)*SSALB(lyr)*FLYR(lyr);
    stau += DTAUC(lyr);
  }

  if (wbar <= tiny || fbar <= tiny || stau <= tiny || ds->bc.fbeam <= tiny) {
    return 0.;
  }

  fbar /= wbar;
  wbar /= stau;
  /*
   * Calculate pspike = (2p"-p"*p")
   */
  pspike = 1.;
  gbar   = 1.;
  plm1   = 1.;
  plm2   = 0.;
  /*
   * pspike for l <= 2n-1
   */
  for (k = 1; k <= ds->nstr-1; k++) {
    pl      = ((double)(2*k-1)*ctheta*plm1-(double)(k-1)*plm2)/k;
    plm2    = plm1;
    plm1    = pl;
    pspike += gbar*(2.-gbar)*(double)(2*k+1)*pl;
  }
  /*
   * pspike for l > 2n-1
   */
  for (k = ds->nstr; k <= ds->nmom; k++) {
    pl   = ((double)(2*k-1)*ctheta*plm1-(double)(k-1)*plm2)/k;
    plm2 = plm1;
    plm1 = pl;
    dtau = UTAU(lu)-TAUC(layru-1);
    gbar = PMOM(k,layru)*SSALB(layru)*dtau;
    for (lyr = 1; lyr <= layru-1; lyr++) {
      gbar += PMOM(k,lyr)*SSALB(lyr)*DTAUC(lyr);
    }
    tmp = fbar*wbar*stau;
    if (tmp <= tiny) {
      gbar = 0.;
    }
    else {
      gbar /= tmp;
    }
    pspike += gbar*(2.-gbar)*(double)(2*k+1)*pl;
  }
  umu0p = ds->bc.umu0/(1.-fbar*wbar);
  /*
   * Calculate IMS correction term, eq. STWL (A.13)
   */
  return ds->bc.fbeam/(4.*M_PI)*SQR(fbar*wbar)/(1.-fbar*wbar)*pspike*c_xi_func(-UMU(iu),umu0p,UTAU(lu));
}

/*============================= end of c_secondary_scat() ===============*/

/*============================= c_new_intensity_correction() ============*/

/*
       Corrects intensity field by using alternative Buras-Emde algorithm
       (201X).

                I N P U T   V A R I A B L E S

       ds      Disort state variables
       dither  small multiple of machine precision
       flyr    separated fraction in delta-M method
       layru   index of UTAU in multi-layered system
       lyrcut  logical flag for truncation of computational layer
       ncut    total number of computational layers considered
       oprim   delta-M-scaled single-scatter albedo
       phirad  azimuthal angles in radians
       tauc    optical thickness at computational levels
       taucpr  delta-M-scaled optical thickness
       utaupr  delta-M-scaled version of UTAU

                O U T P U T   V A R I A B L E S

       out->UU  corrected intensity field; UU(IU,LU,J)
                 iu=1,ds->numu; lu=1,ds->ntau; j=1,ds->nphi

                I N T E R N A L   V A R I A B L E S

       ctheta    cosine of scattering angle
       dtheta    angle (degrees) to define aureole region as
                      direction of beam source +/- DTHETA
       phasa     actual (exact) phase function
       phasm     delta-M-scaled phase function
       phast     phase function used in TMS correction; actual phase
                      function divided by (1-FLYR*SSALB)
       pl        ordinary Legendre polynomial of degree l, P-sub-l
       plm1      ordinary Legendre polynomial of degree l-1, P-sub-(l-1)
       plm2      ordinary Legendre polynomial of degree l-2, P-sub-(l-2)
       theta0    incident zenith angle (degrees)
       thetap    emergent angle (degrees)
       ussndm    single-scattered intensity computed by using exact
                     phase function and scaled optical depth
                     (first term in STWL(68a))
       ussp      single-scattered intensity from delta-M method
                     (second term in STWL(68a))
       duims     intensity correction term from IMS method
                     (delta-I-sub-IMS in STWL(A.19))
       nf        number of angular phase integration grid point
                     (zenith angle, theta)
       np        number of angular phase integration grid point
                     (azimuth angle, phi)
       nphase    number of angles for which original phase function
                     (ds->phase) is defined
       mu_eq     cos(theta) phase integration grid points,
                     equidistant in abs(f_phas2)
       norm_phas normalization factor for phase integration
       norm      normalization factor for preparation of phas2
       neg_phas  index whether phas2 is negative
       phas2     residual phase function
       phasr     delta-M scaled phase function
       f_phas2   cumulative integrated phase function phas2
       fbar      mean value of separated fraction f

   Called by- c_disort
   Calls- c_single_scat, c__new_secondary_scat,
          prep_double_scat_integr, c_dbl_vector
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_new_intensity_correction(disort_state  *ds,
				disort_output *out,
				double         dither,
				double        *flyr,
				int           *layru,
				int            lyrcut,
				int            ncut,
				double        *oprim,
				double        *phasa,
				double        *phast,
				double        *phasm,
				double        *phirad,
				double        *tauc,
				double        *taucpr,
				double        *utaupr)
{
  int
    iu,jp,k,lc,ltau,lu;
  double
    ctheta,dtheta,duims,pl,plm1,plm2,
    theta0=0,thetap=0,ussndm,ussp;

  const int
    nf = 100;
  const double
    tiny = 1e-4;
  int it=0, lyr=0;
  int nphase=ds->nphase;

  double *mu_eq=NULL, *norm_phas=NULL, norm=0.0;
  int *neg_phas=NULL;

  double *phas2=NULL, *phasr=NULL;
  double f_phas2=0.0;
  double fbar=0.0;
  int need_secondary_scattering=0;

  dtheta = 10.;

  /* beginning of BDE stuff */

  /* check whether secondary scattering is performed at all */
  for (iu = 1; iu <= ds->numu; iu++) {
    if (UMU(iu) < 0.) {
      /*
       * Calculate zenith angles of incident and emerging directions
       */
      theta0 = acos(-ds->bc.umu0)/DEG;
      thetap = acos(UMU(iu))/DEG;
      if (fabs(theta0-thetap) <= dtheta) {
	need_secondary_scattering=TRUE;
	break;
      }
    }
  }

  if (need_secondary_scattering==TRUE) {
    /* Initialization of new PSPIKE.                                      */

    mu_eq  = c_dbl_vector(0,nf*ds->ntau-1,"mu_eq");
    norm_phas = c_dbl_vector(0,ds->ntau-1,"norm_phas");
    neg_phas  = c_int_vector(0,nf*ds->ntau-1,"neg_phas");
    phas2 = c_dbl_vector(0,ds->nphase*ds->ntau-1,"phas2");
    phasr = c_dbl_vector(0,ds->nlyr-1,"phasr");

    /* Calculate delta-scaled phase function (phasr) */

    for (it=1; it<=ds->nphase; it++) {

      ctheta = ds->MUP(it);

      for (lc=1; lc<=ds->nlyr; lc++)
	PHASR(lc) = 1.0 - FLYR(lc);

      plm1 = 1.0;
      plm2 = 0.0;

      for (k=1; k<=ds->nstr-1; k++) {

	/* ** Calculate Legendre polynomial of */
	/* ** P-sub-l by upward recurrence     */

	pl = ( (2*k-1) * ctheta * plm1 - (k-1) * plm2 ) / k;
	plm2 = plm1;
	plm1 = pl;

	for (lc=1; lc<=ds->nlyr; lc++)
	  PHASR(lc) += (2*k+1) * pl * ( PMOM(k,lc) - FLYR(lc) );

      }

      /* calculate difference between original and delta-scaled phase
	 functions (phas2) */

      for (lu=1; lu<=ds->ntau; lu++) {

	PHAS2(it,lu) = 0.0;

	/* this could be optimized */
	for (lyr=1; lyr<=LAYRU(lu)-1; lyr++)
	  PHAS2(it,lu) += ( DSPHASE(it,lyr) - PHASR(lyr) ) *
	    SSALB(lyr) * DTAUC(lyr);

	lyr = LAYRU(lu);
	PHAS2(it,lu) += ( DSPHASE(it,lyr) - PHASR(lyr) ) *
	  SSALB(lyr) * ( UTAU(lu) - TAUC(lyr-1) );

      }

    } /* end for it<nphas */

    /* normalize by 1/(ssa*beta*f) */

    for (lu=1; lu<=ds->ntau; lu++) {

      lyr = LAYRU(lu);
      fbar = FLYR(lyr) * SSALB(lyr) * ( UTAU(lu) - TAUC(lyr-1) );

      for (lyr=1; lyr<=LAYRU(lu)-1; lyr++)
	fbar += SSALB(lyr) * DTAUC(lyr) * FLYR(lyr);

      if ( fbar <= tiny || ds->bc.fbeam <= tiny )
	for (it=1; it<=ds->nphase; it++)
	  PHAS2(it,lu) = 0.0;
      else {
	fbar = 1. / fbar;
	for (it=1; it<=ds->nphase; it++)
	  PHAS2(it,lu) *= fbar;
      }

      /* normalize phas2 to 2.0 */

      f_phas2 = 0.0;
      for (it=2; it<=ds->nphase; it++)
	f_phas2 +=
	  ( ds->MUP(it) - ds->MUP(it-1) ) * 0.5 *
	  ( PHAS2(it,lu) + PHAS2(it-1,lu) );

      if (f_phas2 != 0.0) {
	norm = 2.0 / f_phas2;
	for (it=1; it<=ds->nphase; it++)
	  PHAS2(it,lu) *= norm;
      }

    } /* end for lu<ntau */

    prep_double_scat_integr (ds->nphase, ds->ntau, nf, ds->mu_phase,
			     phas2, mu_eq, neg_phas, norm_phas);
  } /* end if (need_secondary_scattering) */

  /* end of BDE stuff */

  /*
   * Start loop over zenith angles
   */
  for (iu = 1; iu <= ds->numu; iu++) {
    if (UMU(iu) < 0.) {
      /*
       * Calculate zenith angles of incident and emerging directions
       */
      theta0 = acos(-ds->bc.umu0)/DEG;
      thetap = acos(UMU(iu))/DEG;
    }
    /*
     * Start loop over azimuth angles
     */
    for (jp = 1; jp <= ds->nphi; jp++) {
      /*
       * Calculate cosine of scattering angle, eq. STWL(4)
       */
      ctheta = -ds->bc.umu0*UMU(iu)+sqrt((1.-SQR(ds->bc.umu0))*(1.-SQR(UMU(iu))))*cos(PHIRAD(jp));
      /*
       * Initialize phase function                              
       */
      for (lc = 1; lc <= ncut; lc++) {
        PHASM(lc) = 1.;
      }

      /* BDE ** Interpolate original phase function */
      /* BDE ** to actual phase function            */

      /* !!! +1: locate starts counting from 0! */
      it = locate_disort ( ds->mu_phase, ds->nphase, ctheta ) + 1;

      for (lc=1; lc<=ncut; lc++)
	PHASA(lc) = DSPHASE(it,lc)
	  + ( ctheta - ds->MUP(it) ) /
	  ( ds->MUP(it+1) - ds->MUP(it) ) *
	  ( DSPHASE(it+1,lc) - DSPHASE(it,lc) );
      /*
       * Initialize Legendre poly. recurrence
       */
      plm1 = 1.;
      plm2 = 0.;
      for (k = 1; k <= ds->nstr-1; k++) {
        /*
         * Calculate Legendre polynomial of P-sub-l by upward recurrence
         */
        pl   = ((double)(2*k-1)*ctheta*plm1-(double)(k-1)*plm2)/k;
        plm2 = plm1;
        plm1 = pl;

        /*
         * Calculate delta-M transformed phase function
         */
	for (lc=1; lc <= ncut; lc++) {
	  PHASM(lc) += (double)(2*k+1)*pl*(PMOM(k,lc)-FLYR(lc))/(1.-FLYR(lc));
	}
      }
      /*
       * Apply TMS method, eq. STWL(68)
       */
      for (lc = 1; lc <= ncut; lc++) {
        PHAST(lc) = PHASA(lc)/(1.-FLYR(lc)*SSALB(lc));
      }
      for (lu = 1; lu <= ds->ntau; lu++) {
        if (!lyrcut || LAYRU(lu) < ncut) {
          ussndm        = c_single_scat(dither,LAYRU(lu),ncut,phast,ds->ssalb,taucpr,UMU(iu),ds->bc.umu0,UTAUPR(lu),ds->bc.fbeam);
          ussp          = c_single_scat(dither,LAYRU(lu),ncut,phasm,oprim,    taucpr,UMU(iu),ds->bc.umu0,UTAUPR(lu),ds->bc.fbeam);
          UU(iu,lu,jp) += ussndm-ussp;
        }
      }
      if (UMU(iu) < 0. && fabs(theta0-thetap) <= dtheta) {
        /*
         * Emerging direction is in the aureole (theta0 +/- dtheta).
         * Apply IMS method for correction of secondary scattering below top level.
         */
        ltau = 1;
        if (UTAU(1) <= dither) {
          ltau = 2;
        }
        for (lu = ltau; lu <= ds->ntau; lu++) {
          if(!lyrcut || LAYRU(lu) < ncut) {
            duims = c_new_secondary_scat(ds,iu,lu,it,ctheta,flyr,
					 LAYRU(lu),tauc,
					 nf,
					 phas2, mu_eq, neg_phas,
					 NORM_PHAS(lu));
	    UU(iu,lu,jp) -= duims;
          }
        }
      } 
    } /* end loop over azimuth angles */
  } /* end loop over zenith angles */

  pfree(mu_eq); pfree(norm_phas); pfree(neg_phas);
  pfree(phas2); pfree(phasr);

  return;
}

/*============================= end of c_new_intensity_correction() =====*/

/*============================= prep_double_scat_integr () ==============*/

/*
       Prepares double scattering integration according to alternative
       Buras-Emde algorithm(201X).

                I N P U T   V A R I A B L E S

       nphase    number of angles for which original phase function
                     (ds->phase) is defined
       ntau      
       nf        number of angular phase integration grid point
                     (zenith angle, theta)
       mu_phase  cos(theta) grid of phase function
       phas2     residual phase function

                O U T P U T   V A R I A B L E S

       mu_eq     cos(theta) phase integration grid points,
                     equidistant in abs(f_phas2)
       neg_phas  index whether phas2 is negative
       norm_phas normalization factor for phase integration

                I N T E R N A L   V A R I A B L E S

       f_phas2_abs absolute value of integrated phase function
                      phas2
       f_phas2     cumulative integrated phase function phas2
       df          step length for calculating mu_eq

   Called by- c_new_intensity_correction
   Calls- c_dbl_vector, locate
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void prep_double_scat_integr (int nphase, int ntau,
			      int           nf,
			      double       *mu_phase,
			      double       *phas2,
			      double       *mu_eq,
			      int          *neg_phas,
			      double       *norm_phas)
{
  int it=0, i=0, lu=0;
  double *f_phas2_abs=NULL;
  double f_phas2=0.0, df=0.0;

  f_phas2_abs = c_dbl_vector(0,nphase,"f_phas2_abs");

  for (lu=1; lu<=ntau; lu++) {

    /* calculate integral of |phas2| (f_phas2_abs) */

    F_PHAS2_ABS(1) = 0.0;
    for (it=2; it<=nphase; it++)
      F_PHAS2_ABS(it) = F_PHAS2_ABS(it-1) +
	( MUP(it) - MUP(it-1) ) * 0.5 *
	( fabs( PHAS2(it,lu) ) + fabs ( PHAS2(it-1,lu) ) );

    /* define mu grid which is equidistant in f_phas2_abs (mu_eq);
       find areas of negative phas2 (neg_phas);
       define normalization (norm_phas) */

    f_phas2 = 0.0;
    df = F_PHAS2_ABS(nphase) / (nf-1);
    MU_EQ(1,lu) = -1.0;

    if ( PHAS2(1,lu) > 0.0 )
      NEG_PHAS(1,lu) = FALSE;
    else
      NEG_PHAS(1,lu) = TRUE;

    it = 1;
    for (i=2; i<=nf-1; i++) {
      f_phas2 += df;

      while ( F_PHAS2_ABS(it+1) < f_phas2 )
	it++;

      MU_EQ(i,lu) = MUP(it)
	+ ( f_phas2 - F_PHAS2_ABS(it) ) /
	( F_PHAS2_ABS(it+1) - F_PHAS2_ABS(it) ) *
	( MUP(it+1) - MUP(it) );

      if ( PHAS2(it,lu) > 0.0 && PHAS2(it+1,lu) > 0.0 )
	NEG_PHAS(i,lu) = FALSE;
      else {
	if ( PHAS2(it,lu) < 0.0 && PHAS2(it+1,lu) < 0.0 )
	  NEG_PHAS(i,lu) = TRUE;
	else {
	  if ( PHAS2(it,lu) + ( f_phas2 - F_PHAS2_ABS(it) ) /
	       ( F_PHAS2_ABS(it+1) - F_PHAS2_ABS(it) ) *
	       ( PHAS2(it+1,lu) - PHAS2(it,lu) ) > 0.0 )
	    NEG_PHAS(i,lu) = FALSE;
	  else
	    NEG_PHAS(i,lu) = TRUE;
	}
      }

    } /* end for i<nf */

    MU_EQ(nf,lu) = 1.0;
    if ( PHAS2(nphase,lu) > 0.0 )
      NEG_PHAS(nf,lu) = FALSE;
    else
      NEG_PHAS(nf,lu) = TRUE;

    NORM_PHAS(lu) = F_PHAS2_ABS(nphase) / ( (nf-1) * M_PI );

  } /* end for lu<ntau */

  pfree(f_phas2_abs);
}

/*============================= end of prep_double_scat_integr() ========*/

/*============================= c_new_secondary_scat() ==================*/

/*
   Calculates secondary scattered intensity, new method (see BDE)

                I N P U T   V A R I A B L E S

        ds        Disort state variables
        iu        index of user polar angle
        lu        index of user level
	it	  index where ctheta contained in mu grid of exact
	             phase function
        ctheta    cosine of scattering angle
        flyr      separated fraction f in Delta-M method
        layru     index of utau in multi-layered system
        tauc      cumulative optical depth at computational layers
        nf        number of angular phase integration grid point
                     (zenith angle, theta)
        phas2     residual phase function
        mu_eq     cos(theta) phase integration grid points,
                     equidistant in abs(f_phas2)
        neg_phas  index whether phas2 is negative
        norm_phas normalization factor for phase integration

                I N T E R N A L   V A R I A B L E S

        pspike  2*p"-p"*p", where p" is the residual phase function
        pspike1 2*p", where p" is the residual phase function
        pspike2 p"*p", where p" is the residual phase function
        wbar    mean value of single scattering albedo
        fbar    mean value of separated fraction f
        dtau    layer optical depth
        stau    sum of layer optical depths between top of atmopshere and layer layru
	umu0p
        nphase  number of angles for which original phase function
                   (ds->phase) is defined

   Called by- c_new_intensity_correction
   Calls- calc_phase_squared, c_xi_func
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline double c_new_secondary_scat(disort_state *ds,
			    int           iu,
			    int           lu,
			    int           it,
			    double        ctheta,
			    double       *flyr,
			    int           layru,
			    double       *tauc,
			    int           nf,
			    double       *phas2,
			    double       *mu_eq,
			    int          *neg_phas,
			    double        norm_phas)
{
  int
    lyr;
  const double
    tiny = 1.e-4;
  double
    dtau,fbar,pspike,
    stau,umu0p,wbar;
  int nphase=ds->nphase;

  double pspike1=0.0, pspike2=0.0;

  /*
   * Calculate vertically averaged value of single scattering albedo and separated
   * fraction f, eq. STWL (A.15)
   */
  dtau = UTAU(lu)-TAUC(layru-1);
  wbar = SSALB(layru)*dtau;
  fbar = FLYR(layru)*wbar;
  stau = dtau;
  for (lyr = 1; lyr <= layru-1; lyr++) {
    wbar += DTAUC(lyr)*SSALB(lyr);
    fbar += DTAUC(lyr)*SSALB(lyr)*FLYR(lyr);
    stau += DTAUC(lyr);
  }

  if (wbar <= tiny || fbar <= tiny || stau <= tiny || ds->bc.fbeam <= tiny) {
    return 0.;
  }

  fbar /= wbar;
  wbar /= stau;

  /* Calculate pspike1=P" */

  pspike1 = PHAS2(it,lu) + ( ctheta - ds->MUP(it) ) /
    ( ds->MUP(it+1) - ds->MUP(it) ) * ( PHAS2(it+1,lu) - PHAS2(it,lu) );

  pspike2 = calc_phase_squared (ds->nphase, lu, ctheta, nf,
				ds->mu_phase, phas2, mu_eq, neg_phas,
				norm_phas);

  pspike = 2.*pspike1 - pspike2;

  umu0p = ds->bc.umu0/(1.-fbar*wbar);

  /*
   * Calculate IMS correction term, eq. STWL (A.13)
   */
  return ds->bc.fbeam/(4.*M_PI)*SQR(fbar*wbar)/(1.-fbar*wbar)*pspike*c_xi_func(-UMU(iu),umu0p,UTAU(lu));
}

/*============================= end of c_new_secondary_scat() ===========*/

/*============================= calc_phase_squared() ====================*/

/*
   Calculates squared phase function (see BDE)

                I N P U T   V A R I A B L E S

        nphase  number of angles for which original phase function
                   (ds->phase) is defined
        lu        index of user level
        ctheta  cosine of scattering angle
        nf        number of angular phase integration grid point
                     (zenith angle, theta)
        mu_phase  cos(theta) grid of phase function
        phas2     residual phase function
        mu_eq     cos(theta) phase integration grid points,
                     equidistant in abs(f_phas2)
        neg_phas  index whether phas2 is negative
        norm_phas normalization factor for phase integration

                I N T E R N A L   V A R I A B L E S

        pspike2  p"*p", where p" is the residual phase function; return value
	mu1arr
	stheta   corresponding sin of ctheta
	smueq    corresponding sin of mu_eq
	phint    phase function integrated over phi
	scr

   Called by- c_new_secondary_scat
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline double calc_phase_squared (int           nphase,
			   int           lu,
			   double        ctheta,
			   int           nf,
			   double       *mu_phase,
			   double       *phas2,
			   double       *mu_eq,
			   int          *neg_phas,
			   double        norm_phas)
{
  int j=0, k=0, it=0;

  double pspike2=0.0, stheta=0.0;
  double smueq=0.0, phint=0.0;

  double mumin=0.0, mumax=0.0;
  int imin=0, imax=0;
  double D=0.0, C=0.0, Dp=0.0, Cp=0.0;
  int cutting=FALSE;

  stheta = sqrt( 1.0 - ctheta * ctheta );

  /* calculate pspike2 */

  /* Note: MU_EQ(j.lu) is mu_1; ctheta is mu; MUP(k) is mu_i in BDE(201X) */


  for (j=1;j<=nf;j++) {

    /* special case: second scattering angle does not depend on
       azimuth of first scattering angle */
    if (ctheta==1.0 || MU_EQ(j,lu)==1.0) {
      it = locate_disort ( mu_phase, nphase, MU_EQ(j,lu)*ctheta ) + 1;
      phint = M_PI * ( PHAS2(it,lu)
		       + ( MU_EQ(j,lu)*ctheta - MUP(it) )
		       / ( MUP (it+1) - MUP(it) )
		       * ( PHAS2(it+1,lu) - PHAS2(it,lu) ) );
      if (ctheta==1.0)
	phint /= 2.0;
    }
    else {
      phint = 0.0;

      smueq = sqrt ( 1. - MU_EQ(j,lu)*MU_EQ(j,lu) );

      /* locate integration borders */
      mumin = ctheta *  MU_EQ(j,lu) - stheta * smueq;
      mumax = ctheta *  MU_EQ(j,lu) + stheta * smueq;

      /* cut where mu_1 = mu_2 */
      if (MU_EQ(j,lu) < mumax) {
	mumax = MU_EQ(j,lu);
	cutting=TRUE;
      }
      else
	cutting=FALSE;

      if (mumin<mumax) {
	imin = locate_disort ( mu_phase, nphase, mumin)+1;
	imax = locate_disort ( mu_phase, nphase, mumax)+1;

	k=imin;
	/* assuming SPF is linear in mu */
	D = ( PHAS2(k+1,lu) - PHAS2(k,lu) ) / ( MUP(k+1) - MUP(k) );
	C = PHAS2(k,lu) - MUP(k) * D;

	phint +=  ( D * ctheta * MU_EQ(j,lu) + C ) * M_PI / 2.0;

	for (k=imin+1;k<=imax;k++) {

	  Dp = ( PHAS2(k+1,lu) - PHAS2(k,lu) ) / ( MUP(k+1) - MUP(k) );
	  Cp = PHAS2(k,lu) - MUP(k) * Dp;

	  phint += 
	    ( Dp - D ) * sqrt ( 1.0 - ctheta * ctheta
				- MU_EQ(j,lu) * MU_EQ(j,lu)
				+ 2.0 * ctheta * MU_EQ(j,lu) * MUP(k)
				- MUP(k) * MUP(k) )
	    + ( ( Dp - D )* ctheta * MU_EQ(j,lu) + Cp - C ) *
	    asin ( ( ctheta * MU_EQ(j,lu) - MUP(k) )
		   / ( smueq * stheta ) );

	  D=Dp;
	  C=Cp;
	}

	if (cutting==TRUE)
	  phint += - D * sqrt ( 1.0 - ctheta * ctheta
			      + 2.0 * MU_EQ(j,lu) * MU_EQ(j,lu) *
			      ( ctheta - 1.0 ) )
	    - ( D * ctheta * MU_EQ(j,lu) + C ) *
	    asin ( ( ctheta - 1.0 ) * MU_EQ(j,lu)
		   / ( smueq * stheta ) );
	else
	  phint += ( D * ctheta * MU_EQ(j,lu) + C ) * M_PI / 2.0;
      }
    }

    if (j==1 || j==nf) {
      if ( NEG_PHAS(j,lu) == TRUE )
	pspike2 = pspike2 - 0.5 * phint;
      else
	pspike2 = pspike2 + 0.5 * phint;
    }
    else {
      if ( NEG_PHAS(j,lu) == TRUE )
	pspike2 = pspike2 - phint;
      else
	pspike2 = pspike2 + phint;
    }

  }

  pspike2 *= norm_phas;

  return pspike2;
}

/*============================= end of calc_phase_squared() =============*/

/*============================= c_single_scat() =========================*/

/*
        Calculates single-scattered intensity from eqs. STWL (65b,d,e)

                I N P U T   V A R I A B L E S
        
        dither   small multiple of machine precision
        layru    index of utau in multi-layered system
        nlyr     number of sublayers
        phase    phase functions of sublayers
        omega    single scattering albedos of sublayers
        tau      optical thicknesses of sublayers
        umu      cosine of emergent angle
        umu0     cosine of incident zenith angle
        utau     user defined optical depth for output intensity
        fbeam   incident beam radiation at top


   Called by- c_intensity_correction
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline double c_single_scat(double   dither,
                     int      layru,
                     int      nlyr,
                     double  *phase,
                     double  *omega,
                     double  *tau,
                     double   umu,
                     double   umu0,
                     double   utau,
                     double   fbeam)
{
  int
    lyr;
  double
    ans,exp0,exp1;

  ans  = 0.;
  exp0 = exp(-utau/umu0);

  if (fabs(umu+umu0) <= dither) {
    /*
     * Calculate downward intensity when umu=umu0, eq. STWL (65e)
     */
    for (lyr = 1; lyr <= layru-1; lyr++) {
      ans += OMEGA(lyr)*PHASE(lyr)*(TAU(lyr)-TAU(lyr-1));
    }
    ans = fbeam/(4.*M_PI*umu0)*exp0*(ans+OMEGA(layru)*PHASE(layru)*(utau-TAU(layru-1)));
    return ans;
  }

  if (umu > 0.) {
    /*
     * Upward intensity, eq. STWL (65b)
     */
    for (lyr = layru; lyr <= nlyr; lyr++) {
      exp1  = exp(-((TAU(lyr)-utau)/umu+TAU(lyr)/umu0));
      ans  += OMEGA(lyr)*PHASE(lyr)*(exp0-exp1);
      exp0  = exp1;
    }
  }
  else {
    /*
     * Downward intensity, eq. STWL (65d)
     */
    for (lyr = layru; lyr >= 1; lyr--) {
      exp1  = exp(-((TAU(lyr-1)-utau)/umu+TAU(lyr-1)/umu0));
      ans  += OMEGA(lyr)*PHASE(lyr)*(exp0-exp1);
      exp0  = exp1;
    }
  }
  ans *= fbeam/(4.*M_PI*(1.+umu/umu0));

  return ans;
}

/*============================= end of c_single_scat() ==================*/

/*============================= c_user_intensities() ====================*/

/*
   Computes intensity components at user output angles for azimuthal
   expansion terms in eq. SD(2), STWL(6)

   I N P U T    V A R I A B L E S:

       ds     :  Disort state variables
       bplanck:  Integrated Planck function for emission from
                 bottom boundary
       cmu    :  Abscissae for Gauss quadrature over angle cosine
       cwt    :  Weights for Gauss quadrature over angle cosine
       delm0  :  Kronecker delta, delta-sub-M0
       emu    :  Surface directional emissivity (user angles)
       expbea :  Transmission of incident beam, EXP(-TAUCPR/UMU0)
       gc     :  Eigenvectors at polar quadrature angles, SC(1)
       gu     :  Eigenvectors interpolated to user polar angles
                    (i.e., G in eq. SC(1) )
       kk     :  Eigenvalues of coeff. matrix in eq. SS(7), STWL(23b)
       layru  :  Layer number of user level UTAU
       ll     :  Constants of integration in eq. SC(1), obtained
                 by solving scaled version of eq. SC(5);
                 exponential term of eq. SC(12) not included
       lyrcut :  Logical flag for truncation of computational layer
       mazim  :  Order of azimuthal component
       ncut   :  Total number of computational layers considered
       nn     :  Order of double-Gauss quadrature (NSTR/2)
       rmu    :  Surface bidirectional reflectivity (user angles)
       taucpr :  Cumulative optical depth (delta-M-Scaled)
       tplanck:  Integrated Planck function for emission from
                 top boundary
       utaupr :  Optical depths of user output levels in delta-M
                 coordinates;  equal to UTAU if no delta-M
       zgu    :  General source function at user angles
       zu     :  Z-sub-zero, Z-sub-one in eq. SS(16) interpolated to user angles from an equation derived from SS(16),
                 Y-sub-zero, Y-sub-one on STWL(26b,a); zu[].zero, zu[].one (see cdisort.h)
       zz     :  Beam source vectors in eq. SS(19), STWL(24b)
       zzg    :  Beam source vectors in eq. KS(10)for a general source constant over a layer
       plk    :  Thermal source vectors z0,z1 by solving eq. SS(16),
                 Y-sub-zero,Y-sub-one in STWL(26)
       zbeam  :  Incident-beam source vectors


    O U T P U T    V A R I A B L E S:

       uum    :  Azimuthal components of the intensity in eq. STWJ(5),
                 STWL(6)

    I N T E R N A L    V A R I A B L E S:

       bnddir :  Direct intensity down at the bottom boundary
       bnddfu :  Diffuse intensity down at the bottom boundary
       bndint :  Intensity attenuated at both boundaries, STWJ(25-6)
       dtau   :  Optical depth of a computational layer
       lyrend :  End layer of integration
       lyrstr :  Start layer of integration
       palint :  Intensity component from parallel beam
       plkint :  Intensity component from planck source
       wk     :  Scratch vector for saving exp evaluations

       All the exponential factors (exp1, expn,... etc.)
       come from the substitution of constants of integration in
       eq. SC(12) into eqs. S1(8-9).  They all have negative
       arguments so there should never be overflow problems.

   Called by- c_disort
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_user_intensities(disort_state   *ds,
                        double          bplanck,
                        double         *cmu,
                        double         *cwt,
                        double          delm0,
                        double         *dtaucpr,
                        double         *emu,
                        double         *expbea,
                        double         *gc,
                        double         *gu,
                        double         *kk,
                        int            *layru,
                        double         *ll,
                        int             lyrcut,
                        int             mazim,
                        int             ncut,
                        int             nn,
                        double         *rmu,
                        double         *taucpr,
                        double          tplanck,
                        double         *utaupr,
                        double         *wk,
			disort_triplet *zbu,
                        double         *zbeam,
			disort_pair    *zbeamsp,
                        double         *zbeama,
                        double         *zgu,
                        disort_pair    *zu,
                        double         *zz,
                        double         *zzg,
                        disort_pair    *plk,
                        double         *uum)
{
  int
    negumu,
    iq,iu,jq,lc,lu,lyrend,lyrstr,lyu;
  double
    alfa,bnddfu,bnddir,bndint,
    denom,dfuint,dtau,dtau1,dtau2,
    exp0=0,exp1=0,exp2=0,expn,
    f0n,f1n,fact,genint,
    palint,plkint,sgn;

  /*
   * Incorporate constants of integration into interpolated eigenvectors
   */
  for (lc = 1; lc <= ncut; lc++) {
    for (iq = 1; iq <= ds->nstr; iq++) {
      for (iu = 1; iu <= ds->numu; iu++) {
        GU(iu,iq,lc) *= LL(iq,lc);
      }
    }
  }

  /*
   * Loop over levels at which intensities are desired ('user output levels')
   */
  for (lu = 1; lu <= ds->ntau; lu++) {
    if (ds->bc.fbeam > 0.) {
      exp0 = exp(-UTAUPR(lu)/ds->bc.umu0);
    }
    lyu = LAYRU(lu);
    /*
     * Loop over polar angles at which intensities are desired
     */
    for (iu = 1; iu <= ds->numu; iu++) {
      if (lyrcut && lyu > ncut) {
        continue;
      }
      negumu = (UMU(iu) < 0.);
      if (negumu) {
        lyrstr = 1;
        lyrend = lyu-1;
        sgn    = -1.;
      }
      else {
        lyrstr = lyu+1;
        lyrend = ncut;
        sgn    = 1.;
      }

      /*
       * For downward intensity, integrate from top to LYU-1 in eq. S1(8); for upward,
       * integrate from bottom to LYU+1 in S1(9)
       */
      genint = 0.;
      palint = 0.;
      plkint = 0.;
      for (lc = lyrstr; lc <= lyrend; lc++) {
        dtau = DTAUCPR(lc);
        exp1 = exp((UTAUPR(lu)-TAUCPR(lc-1))/UMU(iu));
        exp2 = exp((UTAUPR(lu)-TAUCPR(lc  ))/UMU(iu));

        if (ds->flag.planck && mazim == 0) {
          /*
           * Eqs. STWL(36b,c, 37b,c)
           */
          f0n     = sgn*(exp1-exp2);
          f1n     = sgn*((TAUCPR(lc-1)+UMU(iu))*exp1
                        -(TAUCPR(lc  )+UMU(iu))*exp2);
          plkint += Z0U(iu,lc)*f0n+Z1U(iu,lc)*f1n;
        }

        if (ds->bc.fbeam > 0.) {
	  if ( ds->flag.spher == TRUE ) {
	    denom  =  sgn*1.0/(ZBAU(iu,lc)*UMU(iu)+1.0);
	    palint += (ZB0U(iu,lc)*denom*(exp(-ZBAU(iu,lc)*TAUCPR(lc-1)) *exp1
					  -exp(-ZBAU(iu,lc)*TAUCPR(lc)) *exp2 ) 
		       +ZB1U(iu,lc)*denom*((TAUCPR(lc-1)+sgn*denom*UMU(iu))
					   *exp(-ZBAU(iu,lc)*TAUCPR(lc-1)) *exp1
					   -(TAUCPR(lc)+sgn*denom*UMU(iu) )
					   *exp(-ZBAU(iu,lc)*TAUCPR(lc))*exp2));
	  }
	  else {
	    denom = 1.+UMU(iu)/ds->bc.umu0;
	    if (fabs(denom) < 0.0001) {
	      /*
	       * L'Hospital limit
	       */
	      expn = (dtau/ds->bc.umu0)*exp0;
	    }
	    else {
	      expn = (exp1*EXPBEA(lc-1)
		      -exp2*EXPBEA(lc  ))*sgn/denom;
	    }
	    palint += ZBEAM(iu,lc)*expn;
	  }
        }
	if ( ds->flag.general_source ) {
          genint += ZGU(iu,lc)*sgn*(exp1-exp2);
	}
        /*
         * KK is negative
         */
        for (iq = 1; iq <= nn; iq++) {
          WK(iq) = exp(KK(iq,lc)*dtau);
          denom  = 1.+UMU(iu)*KK(iq,lc);
          if (fabs(denom) < 0.0001) {
            /*
             * L'Hospital limit
             */
            expn = (dtau/UMU(iu))*exp2;
          }
          else {
            expn = sgn*(exp1*WK(iq)-exp2)/denom;
          }
          palint += GU(iu,iq,lc)*expn;
        }

        /*
         * KK is positive
         */
        for (iq = nn+1; iq <= ds->nstr; iq++) {
          denom = 1.+UMU(iu)*KK(iq,lc);
          if (fabs(denom) < 0.0001) {
            /*
             * L'Hospital limit
             */
            expn = -(dtau/UMU(iu))*exp1;
          }
          else {
            expn = sgn*(exp1-exp2*WK(ds->nstr+1-iq))/denom;
          }
          palint += GU(iu,iq,lc)*expn;
        }
      }

      /*
       * Calculate contribution from user output level to next computational level
       */
      dtau1 = UTAUPR(lu)-TAUCPR(lyu-1);
      dtau2 = UTAUPR(lu)-TAUCPR(lyu  );

      if ((fabs(dtau1) >= 1.e-6 || !negumu) && (fabs(dtau2) >= 1.e-6 ||  negumu)) {
        if(negumu) {
          exp1 = exp(dtau1/UMU(iu));
        }
        else {
          exp2 = exp(dtau2/UMU(iu));
        }
        if (ds->bc.fbeam > 0.) {
	  if ( ds->flag.spher == TRUE ) {
	    if ( negumu ) {	     
	      expn = exp1;
	      alfa = ZBAU(iu,lyu);
	      denom = (-1.0/(alfa*UMU(iu)+1.));	        
	      palint += ZB0U(iu,lyu)*denom*(-exp(-alfa*UTAUPR(lu))
					    + expn*exp(-alfa*TAUCPR(lyu-1)))
		+ZB1U(iu,lyu)*denom*( -(UTAUPR(lu)-UMU(iu)*denom)*exp(-alfa*UTAUPR(lu))
				      +(TAUCPR(lyu-1)-UMU(iu)*denom)*expn*exp(-alfa*TAUCPR(lyu-1)));
	    }
	    else {
	      expn = exp2;
	      alfa = ZBAU(iu,lyu);
	      denom = (1.0/(alfa*UMU(iu)+1.0));
	      palint += ZB0U(iu,lyu)*denom*(exp(-alfa*UTAUPR(lu))
					    -exp(-alfa*TAUCPR(lyu))*expn)
		+ZB1U(iu,lyu)*denom*( (UTAUPR(lu) +UMU(iu)*denom)*exp(-alfa*UTAUPR(lu))
				      -(TAUCPR(lyu)+UMU(iu)*denom)*exp(-alfa*TAUCPR(lyu))*expn );	          
	    }
	  }
	  else {
	    denom = 1.+UMU(iu)/ds->bc.umu0;
	    if (fabs(denom) < 0.0001) {
	      expn = (dtau1/ds->bc.umu0)*exp0;
	    }
	    else if (negumu) {
	      expn = (exp0-EXPBEA(lyu-1)*exp1)/denom;
	    }
	    else {
	      expn = (exp0-EXPBEA(lyu  )*exp2)/denom;
	    }
	    palint += ZBEAM(iu,lyu)*expn;
	  }
        }
	if ( ds->flag.general_source ) {
          if (negumu) {
            expn = exp1;
          }
          else {
            expn = exp2;
          }
          genint += ZGU(iu,lyu)*(1.-expn);
	}
        /*
         * KK is negative
         */
        dtau = DTAUCPR(lyu);
        for (iq = 1; iq <= nn; iq++) {
          denom = 1.+UMU(iu)*KK(iq,lyu);
          if (fabs(denom) < 0.0001) {
            expn = -dtau2/UMU(iu)*exp2;
          }
          else if (negumu) {
            expn = (exp(-KK(iq,lyu)*dtau2)
                   -exp( KK(iq,lyu)*dtau )*exp1)/denom;
          }
          else {
            expn = (exp(-KK(iq,lyu)*dtau2)-exp2)/denom;
          }
          palint += GU(iu,iq,lyu)*expn;
        }

        /*
         * KK is positive
         */
        for (iq = nn+1; iq <= ds->nstr; iq++) {
          denom = 1.+UMU(iu)*KK(iq,lyu);
          if (fabs(denom) < 0.0001) {
            expn = -(dtau1/UMU(iu))*exp1;
          }
          else if (negumu) {
            expn = (exp(-KK(iq,lyu)*dtau1)-exp1)/denom;
          }
          else {
            expn = (exp(-KK(iq,lyu)*dtau1)
                   -exp(-KK(iq,lyu)*dtau )*exp2)/denom;
          }
          palint += GU(iu,iq,lyu)*expn;
        }

        if (ds->flag.planck && mazim == 0) {
          /*
           * Eqs. STWL (35-37) with tau-sub-n-1 replaced by tau for upward, and
           * tau-sub-n replaced by tau for downward directions
           */
          if (negumu) {
            expn = exp1;
            fact = TAUCPR(lyu-1)+UMU(iu);
          }
          else {
            expn = exp2;
            fact = TAUCPR(lyu  )+UMU(iu);
          }
          f0n     = 1.-expn;
          f1n     = UTAUPR(lu)+UMU(iu)-fact*expn;
          plkint += Z0U(iu,lyu)*f0n+Z1U(iu,lyu)*f1n;
        }
      }

      /*
       * Calculate intensity components attenuated at both boundaries.
       * NOTE: no azimuthal intensity component for isotropic surface
       */
      bndint = 0.;
      if (negumu && mazim == 0) {
        bndint = (ds->bc.fisot+tplanck)*exp(UTAUPR(lu)/UMU(iu));
      }
      else if (!negumu) {
        if (lyrcut || ( ds->flag.lamber && mazim > 0 ) ) {
          UUM(iu,lu) = palint+plkint;
          continue;
        }

        for (jq = nn+1; jq <= ds->nstr; jq++) {
          WK(jq) = exp(-KK(jq,ds->nlyr)*DTAUCPR(ds->nlyr));
        }
        bnddfu = 0.;
        for (iq = nn; iq >= 1; iq--) {
          dfuint = 0.;
          for (jq = 1; jq <= nn; jq++) {
            dfuint += GC(iq,jq,ds->nlyr)*LL(jq,ds->nlyr);
          }
          for (jq= nn+1; jq <= ds->nstr; jq++) {
            dfuint += GC(iq,jq,ds->nlyr)*LL(jq,ds->nlyr)*WK(jq);
          }
          if (ds->bc.fbeam > 0.) {
	    if ( ds->flag.spher == TRUE ) {
	      dfuint += exp(-ZBEAMA(ds->nlyr)*TAUCPR(ds->nlyr)) *
		(ZBEAM0(iq,ds->nlyr)+ZBEAM1(iq,ds->nlyr)*TAUCPR(ds->nlyr));
	    }
	    else {
	      dfuint += ZZ(iq,ds->nlyr)*EXPBEA(ds->nlyr);
	    }
          }
	  if ( ds->flag.general_source ) {
	    dfuint += ZZG(iq,ds->nlyr);
	  }
          dfuint += delm0*(ZPLK0(iq,ds->nlyr)+ZPLK1(iq,ds->nlyr)*TAUCPR(ds->nlyr));
          bnddfu += (1.+delm0)*RMU(iu,nn+1-iq)*CMU(nn+1-iq)*CWT(nn+1-iq)*dfuint;
        }
        bnddir = 0.;
        if (ds->bc.fbeam > 0. || ds->bc.umu0 >0.) {
          bnddir = ds->bc.umu0*ds->bc.fbeam/M_PI*RMU(iu,0)*EXPBEA(ds->nlyr);
        }
        bndint = (bnddfu+bnddir+delm0*EMU(iu)*bplanck+ds->bc.fluor)*exp((UTAUPR(lu)-TAUCPR(ds->nlyr))/UMU(iu));
      }
      UUM(iu,lu) = palint+plkint+bndint+genint;
    }
  }

  return;
}

/*============================= end of c_user_intensities() =============*/

/*============================= c_xi_func() =============================*/

/*
   Calculates Xi function of eq. STWL (72)

         I N P U T   V A R I A B L E S

   umu1,2    cosine of zenith angle_1, _2
   tau       optical thickness of the layer

   NOTE: Original Fortran version also had argument umu3, but was only
         called for the case umu2 == umu3, so these two arguments are
         fused together here to reduce conditional testing.

   Called by- c_secondary_scat
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline double c_xi_func(double umu1,
               double umu2,
               double tau)
{
  double
    exp1,x1;

  x1   = (umu2-umu1)/(umu2*umu1);
  exp1 = exp(-tau/umu1);

  if (x1 != 0.) {
    return ((tau*x1-1.)*exp(-tau/umu2)+exp1)/(x1*x1*umu1*umu2);
  }
  else {
    return tau*tau*exp1/(2.*umu1*umu2);
  }
}

/*============================= end of c_xi_func() ======================*/

