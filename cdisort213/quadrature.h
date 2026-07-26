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

/*============================= c_getmom() ==============================*/

/*
 * Calculate phase function Legendre expansion coefficients in various special cases.
 *
 *  INPUT
 *    iphas  Phase function options
 *                       ISOTROPIC: Isotropic
 *                        RAYLEIGH: Rayleigh
 *               HENYEY_GREENSTEIN: Henyey-Greenstein with asymmetry factor GG
 *             HAZE_GARCIA_SIEWART: Haze L as specified by Garcia/Siewert
 *            CLOUD_GARCIA_SIEWART: Cloud C.1 as specified by Garcia/Siewert
 *    gg      Asymmetry factor for Henyey-Greenstein case
 *    nmom    Index of highest Legendre coefficient needed (number of streams 'nstr'
 *            chosen for the discrete ordinate method). Set to -1 for no scattering.
 *  OUTPUT
 *    PMOM(k) Legendre expansion coefficients (k = 0 to nmom)
 *
 * Reference:  Garcia, R. and C. Siewert, 1985: Benchmark Results in Radiative Transfer, 
 *               Transp. Theory and Stat. Physics 14, 437-484, Tables 10 And 17
 */

DISPATCH_MACRO inline void c_getmom(int    iphas,
              double  gg,
              int     nmom,
              double *pmom)
{
  const double cldmom[299] = {
    2.544,3.883,4.568,5.235,5.887,6.457,7.177,7.859,8.494,9.286,9.856,10.615,11.229,11.851,12.503,
    13.058,13.626,14.209,14.660,15.231,15.641,16.126,16.539,16.934,17.325,17.673,17.999,18.329,18.588,
    18.885,19.103,19.345,19.537,19.721,19.884,20.024,20.145,20.251,20.330,20.401,20.444,20.477,20.489,
    20.483,20.467,20.427,20.382,20.310,20.236,20.136,20.036,19.909,19.785,19.632,19.486,19.311,19.145,
    18.949,18.764,18.551,18.348,18.119,17.901,17.659,17.428,17.174,16.931,16.668,16.415,16.144,15.883,
    15.606,15.338,15.058,14.784,14.501,14.225,13.941,13.662,13.378,13.098,12.816,12.536,12.257,11.978,
    11.703,11.427,11.156,10.884,10.618,10.350,10.090,9.827,9.574,9.318,9.072,8.822,8.584,8.340,8.110,
    7.874,7.652,7.424,7.211,6.990,6.785,6.573,6.377,6.173,5.986,5.790,5.612,5.424,5.255,5.075,4.915,
    4.744,4.592,4.429,4.285,4.130,3.994,3.847,3.719,3.580,3.459,3.327,3.214,3.090,2.983,2.866,2.766,
    2.656,2.562,2.459,2.372,2.274,2.193,2.102,2.025,1.940,1.869,1.790,1.723,1.649,1.588,1.518,1.461,
    1.397,1.344,1.284,1.235,1.179,1.134,1.082,1.040,0.992,0.954,0.909,0.873,0.832,0.799,0.762,0.731,
    0.696,0.668,0.636,0.610,0.581,0.557,0.530,0.508,0.483,0.463,0.440,0.422,0.401,0.384,0.364,0.349,
    0.331,0.317,0.301,0.288,0.273,0.262,0.248,0.238,0.225,0.215,0.204,0.195,0.185,0.177,0.167,0.160,
    0.151,0.145,0.137,0.131,0.124,0.118,0.112,0.107,0.101,0.097,0.091,0.087,0.082,0.079,0.074,0.071,
    0.067,0.064,0.060,0.057,0.054,0.052,0.049,0.047,0.044,0.042,0.039,0.038,0.035,0.034,0.032,0.030,
    0.029,0.027,0.026,0.024,0.023,0.022,0.021,0.020,0.018,0.018,0.017,0.016,0.015,0.014,0.013,0.013,
    0.012,0.011,0.011,0.010,0.009,0.009,0.008,0.008,0.008,0.007,0.007,0.006,0.006,0.006,0.005,0.005,
    0.005,0.005,0.004,0.004,0.004,0.004,0.003,0.003,0.003,0.003,0.003,0.003,0.002,0.002,0.002,0.002,
    0.002,0.002,0.002,0.002,0.002,0.001,0.001,0.001,0.001,0.001,0.001,0.001,0.001,0.001,0.001,0.001,
    0.001,0.001,0.001,0.001,0.001,0.001,0.001};
  const double hazelm[82] = {
    2.41260,3.23047,3.37296,3.23150,2.89350,2.49594,2.11361,1.74812,1.44692,1.17714,0.96643,0.78237,
    0.64114,0.51966,0.42563,0.34688,0.28351,0.23317,0.18963,0.15788,0.12739,0.10762,0.08597,0.07381,
    0.05828,0.05089,0.03971,0.03524,0.02720,0.02451,0.01874,0.01711,0.01298,0.01198,0.00904,0.00841,
    0.00634,0.00592,0.00446,0.00418,0.00316,0.00296,0.00225,0.00210,0.00160,0.00150,0.00115,0.00107,
    0.00082,0.00077,0.00059,0.00055,0.00043,0.00040,0.00031,0.00029,0.00023,0.00021,0.00017,0.00015,
    0.00012,0.00011,0.00009,0.00008,0.00006,0.00006,0.00005,0.00004,0.00004,0.00003,0.00003,0.00002,
    0.00002,0.00002,0.00001,0.00001,0.00001,0.00001,0.00001,0.00001,0.00001,0.00001};
  int
    k;

  /* 
   * Screen for invalid inputs
   */
  if (iphas < FIRST_IPHAS || iphas > LAST_IPHAS) {
    c_errmsg("getmom--bad input variable iphas",DS_ERROR);
  }
  if (nmom < 2) {
    c_errmsg("getmom--bad input variable nmom",DS_ERROR);
  }

  pmom[0] = 1.;
  for (k = 1; k < nmom; k++) {
    pmom[k] = 0.;
  }

  switch(iphas) {
    case RAYLEIGH:
      pmom[2] = 0.1;
    break;
    case HENYEY_GREENSTEIN:
      if (gg <= -1. || gg >= 1.) {
        c_errmsg("getmom--bad input variable gg",DS_ERROR);
      }
      for(k = 1; k <= nmom; k++) {
        pmom[k] = pow(gg,(double)k);
      }
    break;
    case HAZE_GARCIA_SIEWERT:
      /* Haze-L phase function */
      for (k = 1; k <= IMIN(82,nmom); k++) {
        pmom[k] = hazelm[k-1]/(double)(2*k+1);
      }
    break;
    case CLOUD_GARCIA_SIEWERT:
      /* Cloud C.1 phase function */
      for (k = 1; k <= IMIN(298,nmom); k++) {
        pmom[k] = cldmom[k-1]/(double)(2*k+1);
      }
    break;
  }

  return;
}

/*============================= end of c_getmom() =======================*/

/*============================= c_legendre_poly() =======================*/

/*
       Computes the normalized associated Legendre polynomial, defined
       in terms of the associated Legendre polynomial Plm = P-sub-l-super-m as

          Ylm(MU) = sqrt( (l-m)!/(l+m)! ) * Plm(MU)

       for fixed order m and all degrees from l = m to TWONM1.
       When m.GT.0, assumes that Y-sub(m-1)-super(m-1) is available
       from a prior call to the routine.

       REFERENCE: Dave, J.V. and B.H. Armstrong, Computations of High-Order
                    Associated Legendre Polynomials, J. Quant. Spectrosc. Radiat. Transfer 10,
                    557-562, 1970. (hereafter D/A)

       METHOD: Varying degree recurrence relationship.

       NOTES:
       (1) The D/A formulas are transformed by setting m=n-1; l=k-1.
       (2) Assumes that routine is called first with  m = 0, then with
           m = 1, etc. up to  m = twonm1.


  I N P U T     V A R I A B L E S:

       nmu    :  Number of arguments of YLM
       m      :  Order of YLM
       maxmu  : 
       twonm1 :  Max degree of YLM
       MU(i)  :  Arguments of YLM (i = 1 to nmu)

       If m > 0, YLM(m-1,i) for i = 1 to nmu is assumed to exist from a prior call.


  O U T P U T     V A R I A B L E:

       YLM(l,i) :  l = m to twonm1, normalized associated Legendre polynomials
                   evaluated at argument MU(i)

   Called by- c_disort, c_albtrans
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_legendre_poly(int     nmu,
                     int     m,
                     int     maxmu,
                     int     twonm1,
                     double *mu,
                     double *ylm)
{
  int
    i,l;
  double
    tmp1,tmp2;

  if (m == 0) {
    /*
     * Upward recurrence for ordinary Legendre polynomials
     */
    for (i = 1; i <= nmu; i++) {
      YLM(0,i) = 1.;
      YLM(1,i) = MU(i);
    }
    for (l = 2; l <= twonm1; l++) {
      for (i = 1; i <= nmu; i++) {
        YLM(l,i) = ((double)(2*l-1)*MU(i)*YLM(l-1,i)-(double)(l-1)*YLM(l-2,i))/l;
      }
    }
  }
  else {
    for (i = 1; i <= nmu; i++) {
      /*
       * Y-sub-m-super-m; derived from D/A eqs. (11,12), STWL(58c)
       */
      YLM(m,i) = -sqrt((1.-1./(2*m))*(1.-SQR(MU(i))))*YLM(m-1,i);

      /*
       * Y-sub-(m+1)-super-m; derived from D/A eqs.(13,14) using eqs.(11,12), STWL(58f)
       */
      YLM(m+1,i) = sqrt(2.*m+1.)*MU(i)*YLM(m,i);
    }
    /*
     * Upward recurrence; D/A eq.(10), STWL(58a)
     */
    for (l = m+2; l <= twonm1; l++) {
      tmp1 = sqrt((l-m  )*(l+m  ));
      tmp2 = sqrt((l-m-1)*(l+m-1));
      for (i = 1; i <= nmu; i++) {
        YLM(l,i) = ((double)(2*l-1)*MU(i)*YLM(l-1,i)-tmp2*YLM(l-2,i))/tmp1;
      }
    }
  }

  return;
}

/*============================= end of c_legendre_poly() ================*/

/*============================= c_gaussian_quadrature() =================*/

/*
   Compute weights and abscissae for ordinary Gaussian quadrature
   on the interval (0,1);  that is, such that
       sum(i=1 to M) ( GWT(i) f(GMU(i)) )
   is a good approximation to integral(0 to 1) ( f(x) dx )

   INPUT :     m        order of quadrature rule

   OUTPUT :    GMU(I)   array of abscissae (I = 1 TO M)
               GWT(I)   array of weights (I = 1 TO M)

   REFERENCE:  Davis, P.J. and P. Rabinowitz, Methods of Numerical
                 Integration, Academic Press, New York, pp. 87, 1975

   METHOD:     Compute the abscissae as roots of the Legendre polynomial P-sub-M using a cubically convergent
               refinement of Newton's method.  Compute the weights from eq. 2.7.3.8 of Davis/Rabinowitz.  Note
               that Newton's method can very easily diverge; only a very good initial guess can guarantee convergence.
               The initial guess used here has never led to divergence even for M up to 1000.

   ACCURACY:   Relative error no better than TOL or computer precision (DBL_EPSILON), whichever is larger

   INTERNAL VARIABLES:
    iter      : Number of Newton Method iterations
    pm2,pm1,p : 3 successive Legendre polynomials
    ppr       : Derivative of Legendre polynomial
    p2pri     : 2nd derivative of Legendre polynomial
    tol       : Convergence criterion for Legendre poly root iteration
    x,xi      : Successive iterates in cubically-convergent version of Newtons Method (seeking roots of Legendre poly.)

   Called by- c_dref, c_disort_set, c_surface_bidir
   Calls- c_errmsg
 -------------------------------------------------------------------*/

/* Maximum allowed iterations of Newton Method */
#define MAXIT 1000

DISPATCH_MACRO inline void c_gaussian_quadrature(int    m,
                           double *gmu,
                           double *gwt)
{
  int
    initialized = FALSE;
  int
    iter,k,lim,nn,np1;
  double
    cona,t,en,nnp1,p=0,p2pri,pm1,pm2,ppr,
    prod,tmp,x,xi;
  double
    tol;

  if (!initialized) {
    tol         = 10.*DBL_EPSILON;
    initialized = TRUE;
  }

  if (m < 1) {
    c_errmsg("gaussian_quadrature--Bad value of m",DS_ERROR);
  }

  if (m == 1) {
    GMU(1) = 0.5;
    GWT(1) = 1.0;
    return;
  }

  en   = (double)m;
  np1  = m+1;
  nnp1 = m*np1;
  cona = (double)(m-1)/(8*m*m*m);
  lim  = m/2;
  for (k = 1; k <= lim; k++) {
    /*
     * Initial guess for k-th root of Legendre polynomial, from Davis/Rabinowitz (2.7.3.3a)
     */
    t = (double)(4*k-1)*M_PI/(4*m+2);
    x = cos(t+cona/tan(t));

    /*
     * Upward recurrence for Legendre polynomials
     */
    for (iter = 1; iter <= MAXIT+1; iter++) {
      if (iter > MAXIT) {
        c_errmsg("gaussian_quadrature--max iteration count",DS_ERROR);
      }
      pm2 = 1.;
      pm1 = x;
      for (nn = 2; nn <= m; nn++) {
        p   = ((double)(2*nn-1)*x*pm1-(double)(nn-1)*pm2)/nn;
        pm2 = pm1;
        pm1 = p;
      }
      /*
       * Newton Method
       */
      tmp   = 1./(1.-x*x);
      ppr   = en*(pm2-x*p)*tmp;
      p2pri = (2.*x*ppr-nnp1*p)*tmp;
      xi    = x-p/ppr*(1.+p/ppr*p2pri/(2.*ppr));
      /*
       * Check for convergence
       */
      if (fabs(xi-x) <= tol) {
        break;
      }
      else {
        x = xi;
      }
    }

    /*
     * Iteration finished--calculate weights, abscissae for (-1,1)
     */
    GMU(k)     = -x;
    GWT(k)     = 2./(tmp*SQR(en*pm2));
    GMU(np1-k) = -GMU(k);
    GWT(np1-k) =  GWT(k);
  }

  /*
   * Set middle abscissa and weight for rules of odd order
   */
  if (m%2 != 0) {
    GMU(lim+1) = 0.;
    prod       = 1.;
    for (k = 3; k <= m; k+=2) {
      prod *= (double)k/(k-1);
    }
    GWT(lim+1) = 2./SQR(prod);
  }
  /*
   * Convert from (-1,1) to (0,1)
   */
  for (k = 1; k <= m; k++) {
    GMU(k) = 0.5*GMU(k)+0.5;
    GWT(k) = 0.5*GWT(k);
  }

  return;
}

#undef MAXIT

/*============================= end of c_gaussian_quadrature() ==========*/

/*============================= c_chapman() ==============================*/

/*
 Calculates the Chapman factor.

 I n p u t       v a r i a b l e s:

      lc        : Computational layer
      taup      :
      tauc      :
      nlyr      : Number of layers in atmospheric model
      zd(lc)    : lc = 0, nlyr. zd(lc) is distance from bottom
                  surface to top of layer lc. zd(nlyr) = 0. km
      dtau_c    : Optical thickness of layer lc (un-delta-m-scaled)
      zenang    : Solar zenith angle as seen from bottom surface
      r         : Radial parameter, see Velinow & Kostov (2001). NOTE: Use the same dimension as zd,
                  for instance both in km.

 O u t p u t      v a r i a b l e s:

      ch        : Chapman-factor. In a pseudo-spherical atmosphere, replace exp(-tau/umu0) by exp(-ch(lc)) in the
                  beam source in

 I n t e r n a l     v a r i a b l e s:

      dhj       : delta-h-sub-j in eq. B2 (DS)
      dsj       : delta-s-sub-j in eq. B2 (DS)
      fact      : =1 for first  sum in eq. B2 (DS)
                  =2 for second sum in eq. B2 (DS)
      rj        : r-sub-j   in eq. B1 (DS)
      rjp1      : r-sub-j+1 in eq. B1 (DS)
      xpsinz    : The length of the line OG in Fig. 1, (DS)

 
 NOTE: Assumes a spherical planet. One might consider generalizing following
       Velinow YPI, Kostov VI, 2001, Generalization on Chapman Function for the Atmosphere of an Oblate Rotating Planet, 
         Comptes Rendus de l'Academie Bulgare des Sciences 54, 29-34.
*/

DISPATCH_MACRO inline double c_chapman(int     lc,
                 double  taup,
                 double *tauc,
                 int     nlyr,
                 double *zd,
                 double *dtau_c,
                 double  zenang,
                 double  r)
{
  int
    id,j;
  double
    zenrad,xp,xpsinz,
    sum,fact,fact2,rj,rjp1,dhj,dsj;

  zenrad = zenang*DEG;
  xp     = r+ZD(lc)+(ZD(lc-1)-ZD(lc))*taup;
  xpsinz = xp*sin(zenrad);

  if (zenang > 90. && xpsinz < r) {
    return 1.e+20;
  }

  /*
   * Find index of layer in which the screening height lies
   */
  id = lc;
  if (zenang > 90.) {
    for (j= lc; j <= nlyr; j++) {
      if (xpsinz < (ZD(j-1)+r) && (xpsinz >= ZD(j)+r)) {
        id = j;
      }
    }
  }

  sum = 0.;
  for (j = 1; j <= id; j++) {
    fact  = 1.;
    fact2 = 1.;
    /*
     * Include factor of 2 for zenang > 90., second sum in eq. B2 (DS)
     */
    if (j > lc) {
      fact = 2.;
    }
    else if (j == lc && lc == id && zenang > 90.) {
      fact2 = -1.;
    }

    rj   = r+ZD(j-1);
    rjp1 = r+ZD(j  );
    if (j == lc && id == lc) {
      rjp1 = xp;
    }

    dhj = ZD(j-1)-ZD(j);
    if (id > lc && j == id) {
      dsj = sqrt(rj*rj-xpsinz*xpsinz);
    }
    else {
      dsj = sqrt(rj*rj-xpsinz*xpsinz)-fact2*sqrt(rjp1*rjp1-xpsinz*xpsinz);
    }
    sum += DTAU_C(j)*fact*dsj/dhj;
  }
  /*
   * Add third term in eq. B2 (DS)
   */
  if (id > lc) {
    dhj  = ZD(lc-1)-ZD(lc);
    dsj  = sqrt(xp*xp-xpsinz*xpsinz)-sqrt(SQR(ZD(lc)+r)-xpsinz*xpsinz);
    sum += DTAU_C(lc)*dsj/dhj;
  }

  return sum;
}

/*============================= end of c_chapman() =======================*/

DISPATCH_MACRO inline double c_chapman_simpler(int     lc,
                 double  taup,
                 int     nlyr,
                 double *zd,
                 double *dtau_c,
                 double  zenang,
                 double  r)
{
  int
    id,j;
  double
    zenrad,xp,xpsinz,
    sum,fact,fact2,rj,rjp1,dhj,dsj;

  zenrad = zenang*DEG;
  xp     = r+ZD(lc)+(ZD(lc-1)-ZD(lc))*taup;
  xpsinz = xp*sin(zenrad);

  if (zenang > 90. && xpsinz < r) {
    return 1.e+20;
  }

  /*
   * Find index of layer in which the screening height lies
   */
  id = lc;
  if (zenang > 90.) {
    for (j= lc; j <= nlyr; j++) {
      if (xpsinz < (ZD(j-1)+r) && (xpsinz >= ZD(j)+r)) {
        id = j;
      }
    }
  }

  sum = 0.;
  for (j = 1; j <= id; j++) {
    fact  = 1.;
    fact2 = 1.;
    /*
     * Include factor of 2 for zenang > 90., second sum in eq. B2 (DS)
     */
    if (j > lc) {
      fact = 2.;
    }
    else if (j == lc && lc == id && zenang > 90.) {
      fact2 = -1.;
    }

    rj   = r+ZD(j-1);
    rjp1 = r+ZD(j  );
    if (j == lc && id == lc) {
      rjp1 = xp;
    }

    dhj = ZD(j-1)-ZD(j);
    if (id > lc && j == id) {
      dsj = sqrt(rj*rj-xpsinz*xpsinz);
    }
    else {
      dsj = sqrt(rj*rj-xpsinz*xpsinz)-fact2*sqrt(rjp1*rjp1-xpsinz*xpsinz);
    }
    sum += DTAU_C(j)*fact*dsj/dhj;
  }
  /*
   * Add third term in eq. B2 (DS)
   */
  if (id > lc) {
    dhj  = ZD(lc-1)-ZD(lc);
    dsj  = sqrt(xp*xp-xpsinz*xpsinz)-sqrt(SQR(ZD(lc)+r)-xpsinz*xpsinz);
    sum += DTAU_C(lc)*dsj/dhj;
  }

  return sum;
}

/*============================= end of c_chapman_simpler() ===============*/

