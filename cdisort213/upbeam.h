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

/*============================= c_bidir_reflectivity() ==================*/

/*
  Supplies surface bi-directional reflectivity.

  NOTE 1: Bidirectional reflectivity in DISORT is defined by eq. 39 in STWL.
  NOTE 2: Both MU and MU0 (cosines of reflection and incidence angles) are positive.

  Translated from fortran to C by Robert Buras; original name BDREF

  INPUT:

    wvnmlo    : Lower wavenumber (inv cm) of spectral interval
    wvnmhi    : Upper wavenumber (inv cm) of spectral interval
    mu        : Cosine of angle of reflection (positive)
    mup       : Cosine of angle of incidence (positive)
    dphi      : Difference of azimuth angles of incidence and reflection
                (radians)
    brdf_type : BRDF type
    brdf      : BRDF input
    callnum   : number of surface calls

  LOCAL VARIABLES:

    ans       :  Return variable
    badmu     :  minimally allowed value for mu1 and mu2
    flxalb    :  
    irmu      :
    rmu       :
    swvnmlo   : value of wvnmlo from last call of this routine
    swvnmhi   : value of wvnmhi from last call of this routine
    srho0     : value of rho0   from last call of this routine
    sk        : value of k      from last call of this routine
    stheta    : value of theta  from last call of this routine
    ssigma    : value of sigma  from last call of this routine
    st1       : value of t1     from last call of this routine
    st2       : value of t2     from last call of this routine
    sscale    : value of scale  from last call of this routine
    siso      : value of iso    from last call of this routine
    svol      : value of vol    from last call of this routine
    sgeo      : value of geo    from last call of this routine
    su10      : value of u10    from last call of this routine
    spcl      : value of pcl    from last call of this routine
    ssal      : value of sal    from last call of this routine

   Called by- c_dref, c_surface_bidir
   Calls- c_dref, c_bidir_reflectivity_hapke,
          c_bidir_reflectivity_rpv, ocean_brdf, ambrals_brdf
-------------------------------------------------------------------------*/

DISPATCH_MACRO inline double c_bidir_reflectivity ( double       wvnmlo,
			      double       wvnmhi,
			      double       mu,
			      double       mup,
			      double       dphi,
			      int          brdf_type,
			      disort_brdf *brdf,
			      int          callnum )
{
  int
    irmu;

  double
    ans, rmu, flxalb;

  /* These were static memo caches of the previous call's inputs in
   * cdisort.c.  As locals they are seeded with NAN so the comparisons
   * below always fail and the setup path (which computes badmu) runs on
   * every call. */
  double
    badmu = 0.0, swvnmlo = NAN, swvnmhi = NAN, srho0 = NAN, sk = NAN,
    stheta = NAN, ssigma = NAN, st1 = NAN, st2 = NAN, sscale = NAN;

#if HAVE_BRDF
    double
    siso = NAN, svol = NAN, sgeo = NAN;
#endif

  ans = 0.0;

  switch (brdf_type) {
  case BRDF_HAPKE:

    ans = c_bidir_reflectivity_hapke ( wvnmlo, wvnmhi, mu, mup, dphi );

    break;
  case BRDF_RPV:
    if ( swvnmlo != wvnmlo      ||
	 swvnmhi != wvnmhi      ||
	 srho0   != brdf->rpv->rho0   ||
	 sk      != brdf->rpv->k      ||	   	   
	 stheta  != brdf->rpv->theta  ||
	 ssigma  != brdf->rpv->sigma  ||
	 st1     != brdf->rpv->t1     ||
	 st2     != brdf->rpv->t2     ||
	 sscale  != brdf->rpv->scale ) {

      swvnmlo = wvnmlo;
      swvnmhi = wvnmhi;
      srho0   = brdf->rpv->rho0;
      sk      = brdf->rpv->k;
      stheta  = brdf->rpv->theta;
      ssigma  = brdf->rpv->sigma;
      st1     = brdf->rpv->t1;
      st2     = brdf->rpv->t2;
      sscale  = brdf->rpv->scale;

      badmu = 0.0;

      for (irmu=100; irmu>=0; irmu--) {

	rmu = ((double)irmu) * 0.01;

	flxalb = c_dref( wvnmlo, wvnmhi, rmu, brdf_type, brdf, callnum );

	if ( flxalb < 0.0 || flxalb > 1.0 ) {
	  badmu = rmu + 0.01;
	  if (badmu > 1.0)
	    badmu = 1.0;
	  fprintf(stderr,"Using %f as limiting mu in RPV \n",badmu);
	  break;
	}
      }
    }

    ans = c_bidir_reflectivity_rpv ( brdf->rpv, mup, mu, dphi, badmu );

    break;
  case BRDF_CAM:

#if HAVE_BRDF
    /* call C tree saving function */
    /*
     * NOTE: Should group brdf->cam input arguments into the single pointer brdf->cam,
     *       in the same manner as brdf->rpv for c_bidir_reflectivity_rpv().
     */
    ans = ocean_brdf ( wvnmlo, wvnmhi, mu, mup, dphi, 
		       brdf->cam->u10, brdf->cam->pcl, brdf->cam->xsal, callnum);

    /* remove BRDFs smaller than 0 */
    if (ans < 0.0)
      ans = 0.0;

    /* check for NaN */
    if ( ans != ans ) {
      fprintf(stderr,"NaN returned from ocean_brdf: %e %e %e %e %e %e %e %e\n",
	      wvnmlo, wvnmhi, mu, mup, dphi, brdf->cam->u10, brdf->cam->pcl, brdf->cam->xsal);
      ans = 1.0;
    }
#else
    c_errmsg("Error, ocean_brdf is not linked with your code!",DS_ERROR);
#endif
    break;
  case BRDF_AMB:

#if HAVE_BRDF
    /* mu = 0 or dmu = 0 cause problems */
    if ( siso != brdf->ambrals->iso ||
	 svol != brdf->ambrals->vol ||
	 sgeo != brdf->ambrals->geo ) {

      siso = brdf->ambrals->iso;
      svol = brdf->ambrals->vol;
      sgeo = brdf->ambrals->geo;

      badmu = 0.0;

      for (irmu=100; irmu>=0; irmu--) {

	rmu = ((double)irmu) * 0.01;

	flxalb = c_dref( wvnmlo, wvnmhi, rmu, brdf_type, brdf, callnum );

	if ( flxalb < 0.0 || flxalb > 1.0 ) {
	  badmu = rmu + 0.01;
	  if (badmu > 1.0)
	    badmu = 1.0;
	  fprintf(stderr,"Using %f as limiting mu in AMBRALS \n",badmu);
	  break;
	}
      }
    }

    /* convert phi to degrees */
    /*    sdphi = dphi;
	  smup  = mup;
	  smu   = mu; probably no longer needed */

    dphi /= DEG;

    if ( badmu > 0.0 ) {
      if ( mu < badmu )
	mu = badmu;
      if ( mup < badmu )
	mup = badmu;
    }

    /*
     * NOTE: Should group brdf->ambrals input arguments into the single pointer brdf->ambrals,
     *       in the same manner as brdf->rpv for c_bidir_reflectivity_rpv().
     */
    ans = ambrals_brdf (brdf->ambrals->iso, brdf->ambrals->vol, brdf->ambrals->geo, mu, mup, dphi);

    /*    dphi = sdphi;
	  mup  = smup;
	  mu   = smu; probably no longer needed */

    /* check for NaN */
    if ( ans != ans ) {
      fprintf(stderr,"NaN returned from ambrals_brdf: %e %e %e %e %e %e %e %e\n",
	      wvnmlo, wvnmhi, mu, mup, dphi, brdf->ambrals->iso, brdf->ambrals->vol, brdf->ambrals->geo);
      ans = 1.0;
    }
#else
    c_errmsg("Error, ambrals_brdf is not linked with your code!",DS_ERROR);
#endif

    break;
  default:
    fprintf(stderr,"bidir_reflectivity--surface BDRF model %d not known",
	    brdf_type);
    c_errmsg("Exiting...",DS_ERROR);
  }

  return ans;
}

/*============================= end of c_bidir_reflectivity() ===========*/

/*============================= c_bidir_reflectivity_hapke() ============*/

/*    
 * Hapke's BRDF model (times Pi/Mu0):
 *   Hapke, B., Theory of reflectance and emittance spectroscopy, Cambridge University Press, 1993, 
 * eq. 8.89 on page 233. Parameters are from Fig. 8.15 on page 231, except for w.
     
  INPUT:

    wvnmlo : Lower wavenumber (inv cm) of spectral interval
    wvnmhi : Upper wavenumber (inv cm) of spectral interval
    mu     : Cosine of angle of reflection (positive)
    mup    : Cosine of angle of incidence (positive)
    dphi   : Difference of azimuth angles of incidence and reflection
                (radians)

  LOCAL VARIABLES:

    iref   : bidirectional reflectance options; 1 - Hapke's BDR model
    b0     : empirical factor to account for the finite size of particles in Hapke's BDR model
    b      : term that accounts for the opposition effect (retroreflectance, hot spot) in Hapke's BDR model
    ctheta : cosine of phase angle in Hapke's BDR model
    gamma  : albedo factor in Hapke's BDR model
    h0     : H(mu0) in Hapke's BDR model
    h      : H(mu) in Hapke's BDR model
    hh     : angular width parameter of opposition effect in Hapke's BDR model
    p      : scattering phase function in Hapke's BDR model
    theta  : phase angle (radians); the angle between incidence and reflection directions in Hapke's BDR model
    w      : single scattering albedo in Hapke's BDR model

   Called by- c_bidir_reflectivity
-------------------------------------------------------------------------*/

DISPATCH_MACRO inline double c_bidir_reflectivity_hapke ( double wvnmlo,
				    double wvnmhi,
				    double mu,
				    double mup,
				    double dphi )
{
  double
    b0,b,ctheta,Xgamm,
    h0,h,hh,p,thetah,w;

  ctheta = mu*mup+sqrt((1.-mu*mu)*(1.-mup*mup))*cos(dphi);
  thetah = acos(ctheta);
  p      = 1.+.5*ctheta;
  hh     =  .06;
  b0     = 1.;
  b      = b0*hh/(hh+tan(.5*thetah));
  w      = 0.6;
  Xgamm  = sqrt(1.-w);
  h0     = (1.+2.*mup)/(1.+2.*Xgamm*mup);
  h      = (1.+2.*mu )/(1.+2.*Xgamm*mu );

  return .25*w*((1.+b)*p+h0*h-1.0)/(mu+mup);
}
  
/*============================= end of c_bidir_reflectivity_hapke() =====*/

/*============================= c_bidir_reflectivity_rpv() ==============*/

/*
  Computes the Rahman, Pinty, Verstraete BRDF.  The incident
  and outgoing cosine zenith angles are MU1 and MU2, respectively,
  and the relative azimuthal angle is PHI.  In this case the incident
  direction is where the radiation is coming from, so MU1>0 and 
  the hot spot is MU2=MU1 and PHI=180 (the azimuth convention is
  different from the original Frank Evans code). 
  The reference is:
  Rahman, Pinty, Verstraete, 1993: Coupled Surface-Atmosphere 
  Reflectance (CSAR) Model. 2. Semiempirical Surface Model Usable 
  With NOAA Advanced Very High Resolution Radiometer Data,
  J. Geophys. Res., 98, 20791-20801.

  Translated from fortran to C by Robert Buras; original name RPV_REFLECTION

  INPUT:

    rho0   :  BRDF rpv: rho0
    k      :  BRDF rpv: k
    theta  :  BRDF rpv: theta
    sigma  :  BRDF rpv snow: sigma
    t1     :  BRDF rpv snow: t1
    t2     :  BRDF rpv snow: t2
    scale  :  BRDF rpv: scale
    mu1    :  Cosine of angle of reflection (positive)
    mu2    :  Cosine of angle of incidence (positive)
    phi    :  Difference of azimuth angles of incidence and reflection
                 (radians)
    badmu  :  minimally allowed value for mu1 and mu2

  LOCAL VARIABLES:

    ans    :  Return value

   Called by- c_bidir_reflectivity
-------------------------------------------------------------------------*/

DISPATCH_MACRO inline double c_bidir_reflectivity_rpv ( rpv_brdf_spec *brdf,
                                  double         mu1,
				  double         mu2,
				  double         phi,
				  double         badmu )
{
  double
    m, f, h, cosphi, sin1, sin2, cosg, tan1, tan2, capg,
    hspot, t, g;
  double ans;

  /* This function needs more checking; some constraints are 
     required to avoid albedos larger than 1; in particular,
     the BDREF is limited to 5 times the hotspot value to
     avoid extremely large values at low polar angles */


  /* Azimuth convention different from Frank Evans:
     Here PHI=0 means the backward direction while 
     while in DISORT PHI=0 means forward. */
  phi = M_PI - phi;

  /* Don't allow mu's smaller than BADMU because 
     the albedo is larger than 1 for those */
  if ( badmu > 0.0 ) {
    if ( mu1 < badmu )
      mu1 = badmu;
    if ( mu2 < badmu )
      mu2 = badmu;
  }

  /* Hot spot */
  hspot = brdf->rho0 * ( pow ( 2.0 * mu1 * mu1 * mu1 , brdf->k - 1.0 ) * 
		   ( 1.0 - brdf->theta ) / ( 1.0 + brdf->theta ) / ( 1.0 + brdf->theta )
		   *  ( 2.0 - brdf->rho0 ) 
		   + brdf->sigma / mu1 ) * ( brdf->t1 * exp ( M_PI * brdf->t2 ) + 1.0 );

  /* Hot spot region */
  /* is this bug??? phi <= 1e-4 would be more sensible ... RPB */
  if (phi == 1e-4 && mu1 == mu2)
    return hspot * brdf->scale;
      
  m = pow ( mu1 * mu2 * ( mu1 + mu2 ) , brdf->k - 1.0 );
  cosphi = cos(phi);
  sin1 = sqrt ( 1.0 - mu1 * mu1 );
  sin2 = sqrt ( 1.0 - mu2 * mu2 );
  cosg = mu1 * mu2 + sin1 * sin2 * cosphi;
  g = acos ( cosg );
  f = ( 1.0 - brdf->theta * brdf->theta ) /
    pow ( 1.0 + 2.0 * brdf->theta * cosg + brdf->theta * brdf->theta , 1.5);

  tan1 = sin1 / mu1;
  tan2 = sin2 / mu2;
  capg = sqrt( tan1 * tan1 + tan2 * tan2 - 2.0 * tan1 * tan2 * cosphi );
  h = 1.0 + ( 1.0 - brdf->rho0 ) / ( 1.0 + capg );
  t = 1.0 + brdf->t1 * exp ( brdf->t2 * ( M_PI - g ) );

  ans = brdf->rho0 * ( m * f * h + brdf->sigma / mu1 ) * t * brdf->scale;
      
 if (ans < 0.0)
   ans = 0.0;

 return ans;
}

/*============================= end of c_bidir_reflectivity_rpv() =======*/

/*============================= c_surface_bidir() =======================*/

/*
       Computes user's' surface bidirectional properties, STWL(41)

   I N P U T     V A R I A B L E S:

       ds     :  Disort input variables
       cmu    :  Computational polar angle cosines (Gaussian)
       delm0  :  Kronecker delta, delta-sub-m0
       mazim  :  Order of azimuthal component
       nn     :  Order of Double-Gauss quadrature (ds->nstr/2)
       callnum:  number of surface calls

    O U T P U T     V A R I A B L E S:

       bdr :  Fourier expansion coefficient of surface bidirectional
                 reflectivity (computational angles)
       rmu :  Surface bidirectional reflectivity (user angles)
       bem :  Surface directional emissivity (computational angles)
       emu :  Surface directional emissivity (user angles)

    I N T E R N A L     V A R I A B L E S:

       dref   :  Directional reflectivity
       gmu    :  The NMUG angle cosine quadrature points on (0,1)
                 NMUG is set in cdisort.h
       gwt    :  The NMUG angle cosine quadrature weights on (0,1)

   Called by- c_disort
   Calls- c_gaussian_quadrature, c_bidir_reflectivity
+---------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_surface_bidir(disort_state *ds,
                     double        delm0,
                     double       *cmu,
                     int           mazim,
                     int           nn,
                     double       *bdr,
                     double       *emu,
                     double       *bem,
                     double       *rmu,
		     int           callnum)
{
  int
    pass1 = TRUE;
  int
    iq,iu,jg,jq,k;
  double
    dref,sum;
  double
    gmu[NMUG],gwt[NMUG];
  
  if (pass1 && !ds->flag.lamber) {
    pass1 = FALSE;
    c_gaussian_quadrature(NMUG/2,gmu,gwt);
    for (k = 1; k <= NMUG/2; k++) {
      GMU(k+NMUG/2) = -GMU(k);
      GWT(k+NMUG/2) =  GWT(k);
    }
  }

  memset(bdr,0,(ds->nstr/2)*((ds->nstr/2)+1)*sizeof(double));
  memset(bem,0,(ds->nstr/2)*sizeof(double));

  /*
   * Compute Fourier expansion coefficient of surface bidirectional reflectance
   * at computational angles eq. STWL (41)
   */
  if (ds->flag.lamber && mazim == 0) {
    for (iq = 1; iq <= nn; iq++) {
      BEM(iq) = 1.-ds->bc.albedo;
      for (jq = 0; jq <= nn; jq++) {
        BDR(iq,jq) = ds->bc.albedo;
      }
    }
  }
  else if (!ds->flag.lamber) {
    for (iq = 1; iq <= nn; iq++) {
      for (jq = 1; jq <= nn; jq++) {
        sum = 0.;
        for (k = 1; k <= NMUG; k++) {
          sum += GWT(k) * 
	    c_bidir_reflectivity ( ds->wvnmlo, ds->wvnmhi, CMU(iq), CMU(jq),
				   M_PI * GMU(k), ds->flag.brdf_type, &ds->brdf, callnum)
	    * cos((double)mazim * M_PI * GMU(k) );
        }
        BDR(iq,jq) = .5*(2.-delm0)*sum;
      }
      if (ds->bc.fbeam > 0.) {
        sum = 0.;
        for(k = 1; k <= NMUG; k++) {
          sum += GWT (k) *
	    c_bidir_reflectivity ( ds->wvnmlo, ds->wvnmhi, CMU(iq), ds->bc.umu0,
				   M_PI * GMU(k), ds->flag.brdf_type, &ds->brdf, callnum )
	    * cos((double)mazim * M_PI * GMU(k) );
        }
        BDR(iq,0) = .5*(2.-delm0)*sum;
      }
    }
    if (mazim == 0) {
      /*
       * Integrate bidirectional reflectivity at reflection polar angle cosines -CMU- and incident angle
       * cosines -GMU- to get directional emissivity at computational angle cosines -CMU-.
       */
      for (iq = 1; iq <= nn; iq++) {
        dref = 0.;
        for (jg = 1; jg <= NMUG; jg++) {
          sum = 0.;
          for (k = 1; k <= NMUG/2; k++) {
            sum += GWT(k) * GMU(k) *
	      c_bidir_reflectivity ( ds->wvnmlo, ds->wvnmhi, CMU(iq), GMU(k),
				     M_PI * GMU(jg), ds->flag.brdf_type, &ds->brdf, callnum );
          }
          dref += GWT(jg)*sum;
        }
        BEM(iq) = 1.-dref;
      }
    }
  }
  /*
   * Compute Fourier expansion coefficient of surface bidirectional reflectance at user angles eq. STWL (41)
   */
  if(!ds->flag.onlyfl && ds->flag.usrang) {
    memset(emu,0,ds->numu*sizeof(double));
    memset(rmu,0,ds->numu*((ds->nstr/2)+1)*sizeof(double));
    for (iu = 1; iu <= ds->numu; iu++) {
      if (UMU(iu) > 0.) {
        if(ds->flag.lamber && mazim == 0) {
          for (iq = 0; iq <= nn; iq++) {
            RMU(iu,iq) = ds->bc.albedo;
          }
          EMU(iu) = 1.-ds->bc.albedo;
        }
        else if (!ds->flag.lamber) {
          for (iq = 1; iq <= nn; iq++) {
            sum = 0.;
            for (k = 1; k <= NMUG; k++) {
              sum += GWT(k) *
		c_bidir_reflectivity ( ds->wvnmlo, ds->wvnmhi, UMU(iu), CMU(iq),
				       M_PI * GMU(k), ds->flag.brdf_type, &ds->brdf, callnum )
		* cos( (double)mazim * M_PI * GMU(k) );
            }
            RMU(iu,iq) = .5*(2.-delm0)*sum;
          }
          if (ds->bc.fbeam > 0.) {
            sum = 0.;
            for (k = 1; k <= NMUG; k++) {
              sum += GWT(k) *
		c_bidir_reflectivity ( ds->wvnmlo, ds->wvnmhi, UMU(iu),
				       ds->bc.umu0, M_PI * GMU(k),
				       ds->flag.brdf_type, &ds->brdf, callnum )
		* cos( (double)mazim * M_PI * GMU(k) );
            }
            RMU(iu,0) = .5*(2.-delm0)*sum;
          }
          if (mazim == 0) {
            /*
             * Integrate bidirectional reflectivity at reflection angle cosines -UMU- and
             * incident angle cosines -GMU- to get directional emissivity at user angle cosines -UMU-.
             */
            dref = 0.;
            for (jg = 1; jg <= NMUG; jg++) {
              sum = 0.;
              for (k = 1; k <= NMUG/2; k++) {
                sum += GWT(k) * GMU(k) *
		  c_bidir_reflectivity ( ds->wvnmlo, ds->wvnmhi, UMU(iu), GMU(k),
					 M_PI*GMU(jg), ds->flag.brdf_type, &ds->brdf, callnum );
              }
              dref += GWT(jg)*sum;
            }
            EMU(iu) = 1.-dref;
          }
        }
      }
    }
  }

  return;
}

/*============================= end of c_surface_bidir() ================*/

/*============================= c_upbeam() ==============================*/

/*
   Finds the incident-beam particular solution of SS(18), STWL(24a)

   I N P U T    V A R I A B L E S:

       ds     :  Disort state variables
       cc     :  C-sub-ij in eq. SS(5)
       cmu    :  Abscissae for Gauss quadrature over angle cosine
       delm0  :  Kronecker delta, delta-sub-m0
       gl     :  Delta-M scaled Legendre coefficients of phase function
                 (including factors 2l+1 and single-scatter albedo)
       mazim  :  Order of azimuthal component
       ylm0   :  Normalized associated Legendre polynomial at the beam angle
       ylmc   :  Normalized associated Legendre polynomial at the quadrature angles

   O U T P U T    V A R I A B L E S:

       zj     :  Right-hand side vector X-sub-zero in SS(19),STWL(24b);
                 also the solution vector Z-sub-zero after solving that system
       zz     :  Permanent storage for zj, but re-ordered

   I N T E R N A L    V A R I A B L E S:

       array  :  Coefficient matrix in left-hand side of eq. SS(19), STWL(24b)
       ipvt   :  Integer vector of pivot indices required by LINPACK
       wk     :  Scratch array required by LINPACK

   Called by- c_disort
   Calls- c_sgeco, c_errmsg, c_sgesl
 -------------------------------------------------------------------*/

#undef  ARRAY
#define ARRAY(iq,jq) array[iq-1+(jq-1)*ds->nstr]

DISPATCH_MACRO inline void c_upbeam(disort_state *ds,
              int           lc,
              double       *array,
              double       *cc,
              double       *cmu,
              double        delm0,
              double       *gl,
              int          *ipvt,
              int           mazim,
              int           nn,
              double       *wk,
              double       *ylm0,
              double       *ylmc,
              double       *zj,
              double       *zz)
{
  int
    iq,jq,k;
  double sum;
#ifndef __CUDA_ARCH__
  double rcond;
#endif

  for (iq = 1; iq <= ds->nstr; iq++) {
    for (jq = 1; jq <= ds->nstr; jq++) {
      ARRAY(iq,jq) = -CC(iq,jq);
    }
    ARRAY(iq,iq) += 1.+CMU(iq)/ds->bc.umu0;
    sum = 0.;
    for (k = mazim; k <=ds->nstr-1; k++) {
      sum += GL(k,lc)*YLMC(k,iq)*YLM0(k);
    }
    ZJ(iq) = (2.-delm0)*ds->bc.fbeam*sum/(4.*M_PI);
  }
  /*
   * Find L-U (lower/upper triangular) decomposition of ARRAY and see if it is nearly singular
   * (NOTE:  ARRAY is altered)
   */
#ifdef __CUDA_ARCH__
  {
    int info;
    c_sgefa(array,ds->nstr,ds->nstr,ipvt,&info);
    if (info != 0) {
      c_errmsg("upbeam--sgefa says matrix is singular",DS_WARNING);
    }
  }
#else
  rcond = 0.;
  c_sgeco(array,ds->nstr,ds->nstr,ipvt,&rcond,wk);

  if (1.+rcond == 1.) {
    c_errmsg("upbeam--sgeco says matrix near singular",DS_WARNING);
  }
#endif

  /*
   * Solve linear system with coeff matrix ARRAY (assumed already L-U decomposed) and R.H. side(s) ZJ;
   * return solution(s) in ZJ
   */
  c_sgesl(array,ds->nstr,ds->nstr,ipvt,zj,0);
  for (iq = 1; iq <= nn; iq++) {
    ZZ(nn+iq,  lc) = ZJ(iq);
    ZZ(nn-iq+1,lc) = ZJ(iq+nn);
  }

  return;
}

/*============================= end of c_upbeam() =======================*/

/*============================= c_upbeam_pseudo_spherical() =============*/

/*

       Finds the particular solution of beam source KS(10-11)

     Routines called:  sgeco, sgesl

   I N P U T     V A R I A B L E S:

       cc     :  capital-c-sub-ij in Eq. SS(5)
       cmu    :  abscissae for gauss quadrature over angle cosine
       xb0    :  EXPansion of beam source function Eq. KS(7)
       xb1    :  EXPansion of beam source function Eq. KS(7)
       xba    :  EXPansion of beam source function Eq. KS(7)
       (remainder are 'disort' input variables)

    O U T P U T    V A R I A B L E S:

       zbs0     :  solution vectors z-sub-zero of Eq. KS(10-11)
       zbs1     :  solution vectors z-sub-one  of Eq. KS(10-11)
       zbsa     :  alfa coefficient in Eq. KS(7)
       zbeam0,  :  permanent storage for -zbs0,zbs1,zbsa-, but rD-ordered
        zbeam1,
        zbeama
 
   I N T E R N A L    V A R I A B L E S:

       array  :  coefficient matrix in left-hand side of Eq. KS(10)
       ipvt   :  integer vector of pivot indices required by *linpack*
       wk     :  scratch array required by *linpack*

   Called by- c_disort
   Calls- c_sgeco, c_errmsg, c_sgesl
 -------------------------------------------------------------------*/

#undef  ARRAY
#define ARRAY(iq,jq) array[iq-1+(jq-1)*ds->nstr]

DISPATCH_MACRO inline void c_upbeam_pseudo_spherical(disort_state *ds,
			       int           lc,
			       double       *array, 
			       double       *cc,
			       double       *cmu, 
			       int          *ipvt, 
			       int           nn,
			       double       *wk,
			       disort_pair  *xb,
			       double       *xba, 
			       disort_pair  *zbs,
			       double       *zbsa,
			       disort_pair  *zbeamsp,
			       double       *zbeama)
{

  int
    iq,jq;
  double
    rcond,rmin;


  for (iq = 1; iq <= ds->nstr; iq++) {
    for (jq = 1; jq <= ds->nstr; jq++) {
      ARRAY(iq,jq) = -CC(iq,jq);
    }
    ARRAY(iq,iq) += 1.+XBA(lc)*CMU(iq);
    *zbsa     = XBA(lc);
    ZBS1(iq) = XB1(iq,lc);
  }

  /*
   * Find L-U (lower/upper triangular) decomposition of ARRAY and see
   * if it is nearly singular
   * (NOTE: ARRAY is altered)
   */

  rcond = 0.;
  c_sgeco(array,ds->nstr,ds->nstr,ipvt,&rcond,wk);

  if (1.+rcond == 1.) {
    c_errmsg("upbeam_pseudo_spherical--sgeco says matrix near singular",
	     DS_WARNING);
  }
     
  rmin = 1.0e-4;
  if ( rcond < rmin ) {
    /*     Dither alpha if rcond to small   */
    if(XBA(lc) ==0.0)       XBA(lc)=0.000000005;

    XBA(lc) = XBA(lc) * 1.00000005;

    for (iq = 1; iq <= ds->nstr; iq++) {
      for (jq = 1; jq <= ds->nstr; jq++) {
	ARRAY(iq,jq) = -CC(iq,jq);
      }	
      ARRAY(iq,iq) += 1.0+XBA(lc)*CMU(iq);
      *zbsa     = XBA(lc);
      ZBS1(iq) = XB1(iq,lc);
    }
    /*     Solve linear equations KS(10-11) with dithered alpha */
    rcond = 0.;
    c_sgeco(array,ds->nstr,ds->nstr,ipvt,&rcond,wk);               
    if (1.+rcond == 1.) {
      c_errmsg("upbeam_pseudo_spherical--sgeco says matrix near singular",
	       DS_WARNING);
    }
  }

  for (iq = 1; iq <= ds->nstr; iq++)  WK(iq) = ZBS1(iq);
  c_sgesl( array, ds->nstr, ds->nstr, ipvt, wk, 0 );
          
  for (iq = 1; iq <= ds->nstr; iq++) {
    ZBS1(iq) = WK(iq);
    ZBS0(iq) = XB0(iq,lc) + CMU(iq) * ZBS1(iq);
  }

  for (iq = 1; iq <= ds->nstr; iq++)  WK(iq) = ZBS0(iq);
  c_sgesl( array, ds->nstr, ds->nstr, ipvt, wk, 0 );
  for (iq = 1; iq <= ds->nstr; iq++)  ZBS0(iq) = WK(iq);

  /*   ... and now some index gymnastic for the inventive ones...  */

  ZBEAMA(lc)            = *zbsa;
  for (iq = 1; iq <= nn; iq++) {
    ZBEAM0( iq+nn, lc )   = ZBS0( iq );
    ZBEAM1( iq+nn, lc )   = ZBS1( iq );
    ZBEAM0( nn+1-iq, lc ) = ZBS0( iq+nn );
    ZBEAM1( nn+1-iq,lc )  = ZBS1( iq+nn );
  }

 return;

}
  

/*============================= end of c_upbeam_pseudo_spherical() ======*/

/*============================= c_upbeam_general_source() ===============*/

/*
   Finds the incident-beam particular solution of SS(18), STWL(24a)

   I N P U T    V A R I A B L E S:

       ds     :  Disort state variables
       cc     :  C-sub-ij in eq. SS(5)
       cmu    :  Abscissae for Gauss quadrature over angle cosine
       delm0  :  Kronecker delta, delta-sub-m0
       gl     :  Delta-M scaled Legendre coefficients of phase function
                 (including factors 2l+1 and single-scatter albedo)
       mazim  :  Order of azimuthal component
       ylm0   :  Normalized associated Legendre polynomial at the beam angle
       ylmc   :  Normalized associated Legendre polynomial at the quadrature angles

   O U T P U T    V A R I A B L E S:

       zjg    :  Right-hand side vector  X-sub-zero in eq. KS(10), also the solution vector
                 Z-sub-zero after solving that system for a general source constant over a layer
       zzg    :  Permanent storage for zjg, but re-ordered

   I N T E R N A L    V A R I A B L E S:

       array  :  Coefficient matrix in left-hand side of eq. SS(19), STWL(24b)
       ipvt   :  Integer vector of pivot indices required by LINPACK
       wk     :  Scratch array required by LINPACK

   Called by- c_disort
   Calls- c_sgeco, c_errmsg, c_sgesl
 -------------------------------------------------------------------*/

#undef  ARRAY
#define ARRAY(iq,jq) array[iq-1+(jq-1)*ds->nstr]

DISPATCH_MACRO inline void c_upbeam_general_source(disort_state *ds,
			     int           lc,
			     int           maz,
			     double       *array,
			     double       *cc,
			     int          *ipvt,
			     int           nn,
			     double       *wk,
			     double       *zjg,
			     double       *zzg)
{
  int
    iq,jq;
  double
    rcond;

  for (iq = 1; iq <= nn; iq++) {
    ZJG(iq )    = GENSRC(maz,lc,nn+iq);
    ZJG(nn+iq)  = GENSRC(maz,lc,nn+1-iq);
  }

  for (iq = 1; iq <= ds->nstr; iq++) {
    for (jq = 1; jq <= ds->nstr; jq++) {
      ARRAY(iq,jq) = -CC(iq,jq);
    }
    ARRAY(iq,iq) = 1 + ARRAY(iq,iq);
  }

  /*
   * Find L-U (lower/upper triangular) decomposition of ARRAY and see if it is nearly singular
   * (NOTE:  ARRAY is altered)
   */
  rcond = 0.;
  c_sgeco(array,ds->nstr,ds->nstr,ipvt,&rcond,wk);

  if (1.+rcond == 1.) {
    c_errmsg("upbeam_general_source--sgeco says matrix near singular",DS_WARNING);
  }

  /*
   * Solve linear system with coeff matrix ARRAY (assumed already L-U decomposed) and R.H. side(s) ZJG;
   * return solution(s) in ZJG
   */
  c_sgesl(array,ds->nstr,ds->nstr,ipvt,zjg,0);
  for (iq = 1; iq <= nn; iq++) {
    ZZG(nn+iq,  lc) = ZJG(iq);
    ZZG(nn-iq+1,lc) = ZJG(iq+nn);
  }
 
  return;
}

/*============================= end of c_upbeam_general_source() ========*/

/*============================= c_upisot() ==============================*/

/*
    Finds the particular solution of thermal radiation of STWL(25)

    I N P U T     V A R I A B L E S:

       ds     :  Disort state variables
       cc     :  C-sub-ij in eq. SS(5), STWL(8b)
       cmu    :  Abscissae for Gauss quadrature over angle cosine
       oprim  :  Delta-M scaled single scattering albedo
       xr     :  Expansion coefficient b-sub-zero, b-sub-one of thermal source function, eq. STWL(24c)

    O U T P U T    V A R I A B L E S:

       zee    :  Solution vectors Z-sub-zero, Z-sub-one of eq. SS(16), STWL(26a,b)
       plk    :  Permanent storage for zee, but re-ordered

   I N T E R N A L    V A R I A B L E S:

       array  :  Coefficient matrix in left-hand side of eq. SS(16)
       ipvt   :  Integer vector of pivot indices required by LINPACK
       wk     :  Scratch array required by LINPACK

   Called by- c_disort
   Calls- c_sgeco, c_errmsg, c_sgesl
 -------------------------------------------------------------------*/

#undef  ARRAY
#define ARRAY(iq,jq) array[iq-1+(jq-1)*ds->nstr]

DISPATCH_MACRO inline void c_upisot(disort_state *ds,
              int           lc,
              double       *array,
              double       *cc,
              double       *cmu,
              int          *ipvt,
              int           nn,
              double       *oprim,
              double       *wk,
              disort_pair  *xr,
              disort_pair  *zee,
              disort_pair  *plk)
{
  int
    iq,jq;
  double
    rcond;

  for (iq = 1; iq <= ds->nstr; iq++) {
    for (jq = 1; jq <= ds->nstr; jq++) {
      ARRAY(iq,jq) = -CC(iq,jq);
    }
    ARRAY(iq,iq) += 1.;
    Z1(iq) = (1.-OPRIM(lc))*XR1(lc);
  }
  /*
   * Solve linear equations: same as in upbeam, except zj replaced by z1 and z0
   */
  rcond = 0.;
  c_sgeco(array,ds->nstr,ds->nstr,ipvt,&rcond,wk);

  if (1.+rcond == 1.) {
    c_errmsg("upisot--sgeco says matrix near singular",DS_WARNING);
  }
  
  for (iq = 1; iq <= ds->nstr; iq++) {
    /* Need to use WK() as a buffer, since Z1 is part of a structure */
    WK(iq) = Z1(iq);
  }
  c_sgesl(array,ds->nstr,ds->nstr,ipvt,wk,0);
  for (iq = 1; iq <= ds->nstr; iq++) {
    Z1(iq) = WK(iq);
  }

  for (iq = 1; iq <= ds->nstr; iq++) {
    Z0(iq) = (1.-OPRIM(lc))*XR0(lc)+CMU(iq)*Z1(iq);
  }

  for (iq = 1; iq <= ds->nstr; iq++) {
    /* Need to use WK() as a buffer, since Z0 is part of a structure */
    WK(iq) = Z0(iq);
  }
  c_sgesl(array,ds->nstr,ds->nstr,ipvt,wk,0);
  for (iq = 1; iq <= ds->nstr; iq++) {
    Z0(iq) = WK(iq);
  }
  for (iq = 1; iq <= nn; iq++) {
    ZPLK0(nn+iq,  lc) = Z0(iq   );
    ZPLK1(nn+iq,  lc) = Z1(iq   );
    ZPLK0(nn-iq+1,lc) = Z0(iq+nn);
    ZPLK1(nn-iq+1,lc) = Z1(iq+nn);
  }

  return;
}

/*============================= end of c_upisot() =======================*/

/*============================= c_dref() ================================*/

/*
  Flux albedo for given angle of incidence, given a bidirectional reflectivity.

  INPUTS
    wvnmlo    :  Lower wavenumber (inv-cm) of spectral interval
    wvnmhi    :  Upper wavenumber (inv-cm) of spectral interval
    mu        :  Cosine of incidence angle
    brdf_type :  BRDF type
    brdf      :  pointer to disort_brdf structure
    callnum   :  number of surface calls

  INTERNAL VARIABLES

    gmu    : The NMUG angle cosine quadrature points on (0,1)
             NMUG is set in cdisort.h
    gwt    : The NMUG angle cosine quadrature weights on (0,1)

   Called by- c_check_inputs
   Calls- c_gaussian_quadrature, c_errmsg, c_bidir_reflectivity
 --------------------------------------------------------------------*/

DISPATCH_MACRO inline double c_dref(double       wvnmlo,
              double       wvnmhi,
              double       mu,
	      int          brdf_type,
	      disort_brdf *brdf,
	      int          callnum )
{
  int
    pass1 = TRUE;
  int
    jg,k;
  double
    ans,sum;
  double
    gmu[NMUG],gwt[NMUG];

  if (pass1) {
    pass1 = FALSE;
    c_gaussian_quadrature(NMUG/2,gmu,gwt);
    for (k = 1; k <= NMUG/2; k++) {
      GMU(k+NMUG/2) = -GMU(k);
      GWT(k+NMUG/2) =  GWT(k);
    }
  }

  if (fabs(mu) > 1.) {
    c_errmsg("dref--input argument error(s)",DS_ERROR);
  }

  ans = 0.;
  /*
   * Loop over azimuth angle difference
   */
  for (jg = 1; jg <= NMUG; jg++) {
    /*
     * Loop over angle of reflection
     */
    sum = 0.;
    for (k = 1; k <= NMUG/2; k++) {
      sum += GWT(k) * GMU(k) *
	c_bidir_reflectivity ( wvnmlo, wvnmhi, GMU(k), mu, M_PI*GMU(jg), brdf_type, brdf, callnum );
    }
    ans += GWT(jg)*sum;
  }
  if (ans < 0. || ans > 1.) {
    c_errmsg("DREF--albedo value not in [0,1]",DS_WARNING);
  }

  return ans;
}

/*============================= end of c_dref() =========================*/
