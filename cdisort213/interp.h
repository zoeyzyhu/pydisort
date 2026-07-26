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

/*============================= c_interp_eigenvec() =====================*/

/*
   Interpolate eigenvectors to user angles; eq SD(8)

   Called by- c_disort, c_albtrans
 --------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_interp_eigenvec(disort_state *ds,
                       int           lc,
                       double       *cwt,
                       double       *evecc,
                       double       *gl,
                       double       *gu,
                       int           mazim,
                       int           nn,
                       double       *wk,
                       double       *ylmc,
                       double       *ylmu)
{
  int
    iq,iu,jq,l;
  double
    sum;

  for (iq = 1; iq <= ds->nstr; iq++) {
    for (l = mazim; l <= ds->nstr-1; l++) {
      /*
       * Inner sum in SD(8) times all factors in outer sum but PLM(mu)
       */
      sum = 0.;
      for (jq = 1; jq <= ds->nstr; jq++) {
        sum += CWT(jq)*YLMC(l,jq)*EVECC(jq,iq);
      }
      WK(l+1) = .5*GL(l,lc)*sum;
    }
    /*
     * Finish outer sum in SD(8) and store eigenvectors
     */
    for (iu = 1; iu <= ds->numu; iu++) {
      sum = 0.;
      for (l = mazim; l <=ds->nstr-1; l++) {
        sum += WK(l+1)*YLMU(l,iu);
      }
      if (iq <= nn) {
        GU(iu,nn+iq,lc) = sum;
      }
      else {
        GU(iu,ds->nstr+1-iq,lc) = sum;
      }
    }
  }

  return;
}

/*============================= end of c_interp_eigenvec() ==============*/

/*============================= c_interp_source() =======================*/

/*
    Interpolates source functions to user angles, eq. STWL(30)

    I N P U T      V A R I A B L E S:

       ds     :  Disort state variables
       cwt    :  Weights for Gauss quadrature over angle cosine
       delm0  :  Kronecker delta, delta-sub-m0
       gl     :  Delta-M scaled Legendre coefficients of phase function
                 (including factors 2l+1 and single-scatter albedo)
       mazim  :  Order of azimuthal component
       oprim  :  Single scattering albedo
       xr     :  Expansion of thermal source function, eq. STWL(24d); xr[].zero, xr[].one (see cdisort.h)
       ylm0   :  Normalized associated Legendre polynomial at the beam angle
       ylmc   :  Normalized associated Legendre polynomial at the quadrature angles
       ylmu   :  Normalized associated Legendre polynomial at the user angles
       zbs0   :  Solution vectors z-sub-zero of Eq. KS(10-11), used if pseudo-spherical
       zbs1   :  Solution vectors z-sub-one  of Eq. KS(10-11), used if pseudo-spherical
       zbsa   :  Alfa coefficient in Eq. KS(7), used if pseudo-spherical
       zee    :  Solution vectors Z-sub-zero, Z-sub-one of eq. SS(16), STWL(26a,b)
       zj     :  Solution vector Z-sub-zero after solving eq. SS(19), STWL(24b)
       zjg    :  Right-hand side vector  X-sub-zero in eq. KS(10), also the solution vector
                 Z-sub-zero after solving that system for a general source constant over a layer

    O U T P U T     V A R I A B L E S:

       zbeam  :  Incident-beam source function at user angles
       zu     :  Components 0 and 1 of a linear-in-optical-depth-dependent source (approximating the Planck emission source)
       zgu    :  General source function at user angles

   I N T E R N A L    V A R I A B L E S:

       psi  :   psi[].zero: Sum just after square bracket in eq. SD(9)
                psi[].one:  Sum in eq. STWL(31d)

   Called by- c_disort
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_interp_source(disort_state   *ds,
                     int             lc,
                     double         *cwt,
                     double          delm0,
                     double         *gl,
                     int             mazim,
                     double         *oprim,
                     double         *ylm0,
                     double         *ylmc,
                     double         *ylmu,
                     disort_pair    *psi,
                     disort_pair    *xr,
                     disort_pair    *zee,
                     double         *zj,
		     double         *zjg,
                     double         *zbeam,
		     disort_triplet *zbu,
		     disort_pair    *zbs,
		     double          zbsa,
		     double         *zgu,
                     disort_pair    *zu)
{
  int
    iq,iu,jq;
  double
    fact,psum,psum0,psum1,sum,sum0,sum1;

  if (ds->bc.fbeam > 0.) {
    /*
     * Beam source terms; eq. SD(9)
     */
    if ( ds->flag.spher == TRUE ) {
      for (iq = mazim; iq <= ds->nstr-1; iq++) {
	psum0 = 0.;
	psum1 = 0.;
	for (jq = 1; jq <= ds->nstr; jq++) {
	  psum0 +=  CWT(jq)*YLMC(iq,jq)*ZBS0(jq);
	  psum1 +=  CWT(jq)*YLMC(iq,jq)*ZBS1(jq);
	}
	PSI0(iq+1) = 0.5*GL(iq,lc)*psum0;
	PSI1(iq+1) = 0.5*GL(iq,lc)*psum1;
      }
      for (iu = 1; iu <= ds->numu; iu++) {
	sum0 = 0.;
	sum1 = 0.;
	for (iq = mazim; iq <= ds->nstr-1; iq++) {
	  sum0 += YLMU(iq,iu)*PSI0(iq+1);
	  sum1 += YLMU(iq,iu)*PSI1(iq+1);
	}
	ZB0U(iu,lc) = sum0 + ZB0U(iu,lc);
	ZB1U(iu,lc) = sum1 + ZB1U(iu,lc);
	ZBAU(iu,lc) = zbsa;
      }
    }
    else {
      for (iq = mazim; iq <= ds->nstr-1; iq++) {
	psum = 0.;
	for (jq = 1; jq <= ds->nstr; jq++) {
	  psum += CWT(jq)*YLMC(iq,jq)*ZJ(jq);
	}
	PSI0(iq+1) = .5*GL(iq,lc)*psum;
      }
      fact = (2.-delm0)*ds->bc.fbeam/(4.*M_PI);
      for (iu = 1; iu <= ds->numu; iu++) {
	sum = 0.;
	for (iq = mazim; iq <= ds->nstr-1; iq++) {
	  sum += YLMU(iq,iu)*(PSI0(iq+1)+fact*GL(iq,lc)*YLM0(iq));
	}
	ZBEAM(iu,lc) = sum;
      }
    }
  }
  if (ds->flag.general_source > 0.) {
    /*
     * General source; eq. SD(9), KS(13)
     */
    for (iq = mazim; iq <= ds->nstr-1; iq++) {
      psum0 = 0.;
      for (jq = 1; jq <= ds->nstr; jq++) {
	psum0 +=  CWT(jq)*YLMC(iq,jq)*ZJG(jq);
      }
      PSI0(iq+1) = 0.5*GL(iq,lc)*psum0;
    }
    for (iu = 1; iu <= ds->numu; iu++) {
      sum0 = 0.;
      for (iq = mazim; iq <= ds->nstr-1; iq++) {
	sum0 += YLMU(iq,iu)*PSI0(iq+1);
      }
      ZGU(iu,lc) = sum0 + GENSRCU(mazim,lc,iu);
    }
  }

  if (ds->flag.planck && mazim == 0) {
    /*
     * Thermal source terms, STWJ(27c), STWL(31c)
     */
    for (iq = mazim; iq <=ds->nstr-1; iq++) {
      psum0 = 0.;
      psum1 = 0.;
      for (jq = 1; jq <= ds->nstr; jq++) {
        psum0 += CWT(jq)*YLMC(iq,jq)*Z0(jq);
        psum1 += CWT(jq)*YLMC(iq,jq)*Z1(jq);
      }
      PSI0(iq+1) = .5*GL(iq,lc)*psum0;
      PSI1(iq+1) = .5*GL(iq,lc)*psum1;
    }
    for (iu = 1; iu <= ds->numu; iu++) {
      sum0 = 0.;
      sum1 = 0.;
      for (iq = mazim; iq <= ds->nstr-1; iq++) {
        sum0 += YLMU(iq,iu)*PSI0(iq+1);
        sum1 += YLMU(iq,iu)*PSI1(iq+1);
      }
      Z0U(iu,lc) = sum0+(1.-OPRIM(lc))*XR0(lc);
      Z1U(iu,lc) = sum1+(1.-OPRIM(lc))*XR1(lc);
    }
  }

  return;
}

/*============================= end of c_interp_source() ================*/


/*============================= c_interp_coefficients_beam_source =======*/

/*
     Find coefficients at user angle, necessary for later use in
     c_interp_source()
*/

/*

    I N P U T      V A R I A B L E S:

       cmu    :   Computational polar angles
       chtau  :   The optical depth in spherical geometry.
       delmo  :   Kronecker delta, delta-sub-m0
       fbeam  :   incident beam radiation at top
       gl     :   Phase function Legendre coefficients multiplied by (2l+1) and single-scatter albedo
       lc:    :   layer index
       mazim  :   order of azimuthal component
       nstr   :   number of streams
       numu   :   number of user angles
       taucpr :   delta-m-scaled optical depth
       xba    :   alfa in eq. KS(7) 
       ylmu   :   Normalized associated Legendre polynomial at the user angles -umu-
       ylm0   :   Normalized associated Legendre polynomial at the beam angle

    O U T P U T     V A R I A B L E S:

       zb0u   :   x-sub-zero in KS(7) at user angles -umu-
       zb1u   :   x-sub-one in KS(7) at user angles -umu-
       zju    :  Solution vector Z-sub-zero after solving eq. SS(19), STWL(24b), at user angles -umu-

   Called by- c_disort

*/

DISPATCH_MACRO inline void c_interp_coefficients_beam_source(disort_state   *ds,
				       double         *chtau,
				       double          delm0,
				       double          fbeam,
				       double         *gl,
				       int             lc,
				       int             mazim,
				       int             nstr,
				       int             numu,
				       double         *taucpr,
				       disort_triplet *zbu,
				       double         *xba,
				       double         *zju,
				       double         *ylm0,
				       double         *ylmu)
{
  int 
    iu,k;
  double 
    deltat,sum,q0a,q2a,q0,q2;
  
  /*     Calculate x-sub-zero in STWJ(6d) */
  deltat = TAUCPR(lc) - TAUCPR(lc-1);

  q0a = exp(-CHTAU(lc-1));
  q2a = exp(-CHTAU(lc));
     
  for (iu = 1; iu <= numu; iu++) {
    sum = 0.0;
    for (k = mazim; k <= nstr-1; k++) {
      sum = sum + GL(k,lc)*YLMU(k,iu)*YLM0(k);
    }
    ZJU(iu) = (2.0-delm0)*fbeam*sum/(4.0*M_PI);
  }

  for (iu = 1; iu <= numu; iu++) {
     
    q0 = q0a*ZJU(iu);
    q2 = q2a*ZJU(iu);
     
    /*     x-sub-zero and x-sub-one in Eqs. KS(48-49)   */

    ZB1U(iu,lc)=(1./deltat)*(q2*exp(XBA(lc)*TAUCPR(lc))
			     -q0*exp(XBA(lc)*TAUCPR(lc-1)));
    ZB0U(iu,lc) = q0*exp(XBA(lc)*TAUCPR(lc-1))-ZB1U(iu,lc)*TAUCPR(lc-1);
  }

  return;

}
/*============================= end c_interp_coefficients_beam_source ===*/

/*============================= c_set_coefficients_beam_source() ========*/

/*
       Set coefficients in ks(7) for beam source

    I N P U T      V A R I A B L E S:

       cmu    :   Computational polar angles
       ch     :   The Chapman-factor to correct for pseudo-spherical geometry in the direct beam.
       chtau  :   The optical depth in spherical geometry.
       delmo  :   Kronecker delta, delta-sub-m0
       fbeam  :   incident beam radiation at top
       gl     :   Phase function Legendre coefficients multiplied by (2l+1) and single-scatter albedo
       lc:    :   layer index
       mazim  :   order of azimuthal component
       nstr   :   number of streams
       taucpr :   delta-m-scaled optical depth
       ylmc   :   Normalized associated Legendre polynomial at the quadrature angles -cmu-
       ylm0   :   Normalized associated Legendre polynomial at the beam angle

    O U T P U T     V A R I A B L E S:

       xba    :   alfa in eq. KS(7) 
       xb0    :   x-sub-zero in KS(7)
       xb1    :   x-sub-one in KS(7)
       zj     :  Solution vector Z-sub-zero after solving eq. SS(19), STWL(24b)

   Called by- c_disort
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_set_coefficients_beam_source(disort_state *ds,
				    double       *ch,
				    double       *chtau,
				    double       *cmu, 
				    double        delm0,
				    double        fbeam,
				    double       *gl,
				    int           lc,
				    int           mazim,
				    int           nstr,
				    double       *taucpr,
				    double       *xba,
				    disort_pair  *xb,
				    double       *ylm0,
				    double       *ylmc,
				    double       *zj)
{

  int 
    iq,k;
  double 
    deltat,sum,q0a,q2a,q0,q2;
  double
    big;

  big    = sqrt(DBL_MAX)/1.e+10;

  /*     Calculate x-sub-zero in STWJ(6d)   */

  for (iq = 1; iq <= nstr; iq++) {
    sum = 0;
    for (k = mazim; k <= nstr-1; k++) {
      sum += GL(k,lc)*YLMC(k,iq)*YLM0(k);
    }
    ZJ(iq) = (2.-delm0)*fbeam*sum/(4.*M_PI);
  }

  q0a = exp( -CHTAU(lc-1) );
  q2a = exp( -CHTAU(lc) );

  /*     Calculate alfa coefficient  */
     
  deltat = TAUCPR(lc) - TAUCPR(lc-1);

  XBA(lc) = 1./CH(lc);
        
  if ( fabs(XBA(lc)) > big  &&  TAUCPR(lc) > 1.)  XBA(lc) = 0.0;

  if( fabs(XBA(lc)*TAUCPR(lc)) > log(big))	  XBA(lc) = 0.0;

  /*     Dither alfa if it is close to one of the quadrature angles */

  if (  fabs(XBA(lc)) > 0.00001 ) {
    for (iq = 1; iq <= nstr/2; iq++) {
      if (fabs((fabs(XBA(lc))-1.0/CMU(iq))/XBA(lc) ) < 0.05 ) XBA(lc) = XBA(lc) * 1.001;      
    }
  }

  for (iq = 1; iq <= nstr; iq++) {

    q0 = q0a * ZJ(iq);
    q2 = q2a * ZJ(iq);
     
    /*     x-sub-zero and x-sub-one in Eqs. KS(48-49)   */   
       
    XB1(iq,lc) = (1.0/deltat)*(q2*exp(XBA(lc)*TAUCPR(lc)) - q0*exp(XBA(lc)*TAUCPR(lc-1)));    
    XB0(iq,lc) = q0 * exp(XBA(lc)*TAUCPR(lc-1)) - XB1(iq,lc)*TAUCPR(lc-1);

  }
  return;
}
/*============================= end c_set_coefficients_beam_source() ====*/


