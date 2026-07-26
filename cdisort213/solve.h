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

/*============================= c_asymmetric_matrix() ===================*/

/*
  Solves eigenfunction problem for real asymmetric matrix for which it
  is known a priori that the eigenvalues are real. This is an adaptation
  of a subroutine EIGRF in the IMSL library to use real instead of complex
  arithmetic, accounting for the known fact that the eigenvalues and
  eigenvectors in the discrete ordinate solution are real.
  
  EIGRF is based primarily on EISPACK routines.  The matrix is first
  balanced using the Parlett-Reinsch algorithm.  Then the Martin-Wilkinson
  algorithm is applied. There is a statement 'j = wk(i)' that converts a
  double precision variable to an integer variable; this seems dangerous
  to us in principle, but seems to work fine in practice.
  
  References:

  Dongarra, J. and C. Moler, EISPACK -- A Package for Solving Matrix
      Eigenvalue Problems, in Cowell, ed., 1984: Sources and Development of
      Mathematical Software, Prentice-Hall, Englewood Cliffs, NJ
  Parlett and Reinsch, 1969: Balancing a Matrix for Calculation of
      Eigenvalues and Eigenvectors, Num. Math. 13, 293-304
  Wilkinson, J., 1965: The Algebraic Eigenvalue Problem, Clarendon Press,
      Oxford

   I N P U T    V A R I A B L E S:

       aa    :  input asymmetric matrix, destroyed after solved
        m    :  order of aa
       ia    :  first dimension of aa
    ievec    :  first dimension of evec

   O U T P U T    V A R I A B L E S:

       evec  :  (unnormalized) eigenvectors of aa (column j corresponds to EVAL(J))
       eval  :  (unordered) eigenvalues of aa (dimension m)
       ier   :  if != 0, signals that EVAL(ier) failed to converge;
                   in that case eigenvalues ier+1,ier+2,...,m  are
                   correct but eigenvalues 1,...,ier are set to zero.

   S C R A T C H   V A R I A B L E S:

       wk    :  work area (dimension at least 2*m)
       
   Called by- c_solve_eigen
   Calls- c_errmsg
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_asymmetric_matrix(double *aa,
                         double *evec,
                         double *eval,
                         int     m,
                         int     ia,
                         int     ievec,
                         int    *ier,
                         double *wk)
{
  const double
   c1 =    .4375,
   c2 =    .5,
   c3 =    .75,
   c4 =    .95,
   c5 =  16.,
   c6 = 256.;
  int
    noconv,notlas,
    i,ii,in,j,k,ka,kkk,l,lb=0,lll,n,n1,n2;
  double
    col,discri,f,g,h,p=0,q=0,r=0,repl,rnorm,row,
    s,scale,sgn,t,tol,uu,vv,w,x,y,z;

  *ier = 0;
  tol = DBL_EPSILON;
  if (m < 1 || ia < m || ievec < m) {
    c_errmsg("asymmetric_matrix--bad input variable(s)",DS_ERROR);
  }

  /*
   * Handle 1x1 and 2x2 special cases
   */
  if (m == 1) {
    EVAL(1)   = AA(1,1);
    EVEC(1,1) = 1.;
    return;
  }
  else if (m == 2) {
    discri = SQR(AA(1,1)-AA(2,2))+4.*AA(1,2)*AA(2,1);
    if(discri < 0.) {
      c_errmsg("asymmetric_matrix--complex evals in 2x2 case",DS_ERROR);
    }
    sgn = 1.;
    if (AA(1,1) < AA(2,2)) {
     sgn = -1.;
    }
    EVAL(1)   = .5*(AA(1,1)+AA(2,2)+sgn*sqrt(discri));
    EVAL(2)   = .5*(AA(1,1)+AA(2,2)-sgn*sqrt(discri));
    EVEC(1,1) = 1.;
    EVEC(2,2) = 1.;
    if (AA(1,1) == AA(2,2) && (AA(2,1) == 0. || AA(1,2) == 0.)) {
      rnorm     = fabs(AA(1,1))+fabs(AA(1,2))+fabs(AA(2,1))+fabs(AA(2,2));
      w         = tol*rnorm;
      EVEC(2,1) =  AA(2,1)/w;
      EVEC(1,2) = -AA(1,2)/w;
    }
    else {
      EVEC(2,1) = AA(2,1)/(EVAL(1)-AA(2,2));
      EVEC(1,2) = AA(1,2)/(EVAL(2)-AA(1,1));
    }
    return;
  }

  /*
   * Initialize output variables
   */
  *ier = 0;
  memset(eval,0,m*sizeof(double));
  memset(evec,0,ievec*ievec*sizeof(double));
  for (i = 1; i <= m; i++) {
    EVEC(i,i) = 1.;
  }

  /*
   * Balance the input matrix and reduce its norm by diagonal similarity transformation stored in wk;
   * then search for rows isolating an eigenvalue and push them down.
   */
  rnorm = 0.;
  l     = 1;
  k     = m;

S50:

  kkk = k;
  for (j = kkk; j >= 1; j--) {
    row = 0.;
    for (i = 1; i <= k; i++) {
      if (i != j) {
        row += fabs(AA(j,i));
      }
    }
    if (row == 0.) {
      WK(k) = (double)j;
      if (j != k) {
        for (i = 1; i <= k; i++) {
          repl    = AA(i,j);
          AA(i,j) = AA(i,k);
          AA(i,k) = repl;
        }
        for (i = l; i <= m; i++) {
          repl    = AA(j,i);
          AA(j,i) = AA(k,i);
          AA(k,i) = repl;
        }
      }
      k--;
      goto S50;
    }
  }

  /*
   * Search for columns isolating an eigenvalue and push them left.
   */

S100:

  lll = l;
  for (j = lll; j <= k; j++) {
    col = 0.;
    for (i = l; i <= k; i++) {
      if (i != j) {
        col += fabs(AA(i,j));
      }
    }
    if (col == 0.) {
      WK(l) = (double)j;
      if (j != l) {
        for (i = 1; i <= k; i++) {
          repl    = AA(i,j);
          AA(i,j) = AA(i,l);
          AA(i,l) = repl;
        }
        for (i = l; i <= m; i++) {
          repl    = AA(j,i);
          AA(j,i) = AA(l,i);
          AA(l,i) = repl;
        }
      }
      l++;
      goto S100;
    }
  }

  /*
   * Balance the submatrix in rows L through K
   */
  for (i = l; i <= k; i++) {
    WK(i) = 1.;
  }

  noconv = TRUE;
  while (noconv) {
    noconv = FALSE;
    for (i = l; i <= k; i++) {
      col = 0.;
      row = 0.;
      for (j = l; j <= k; j++) {
        if (j != i) {
          col += fabs(AA(j,i));
          row += fabs(AA(i,j));
        }
      }

      f = 1.;
      g = row/c5;
      h = col+row;

      while (col < g) {
        f   *= c5;
        col *= c6;
      }

      g = row*c5;

      while (col >= g) {
        f   /= c5;
        col /= c6;
      }

      /*
       * Now balance
       */
      if ((col+row)/f < c4*h) {
        WK(i)  *= f;
        noconv  = TRUE;
        for (j = l; j <= m; j++) {
          AA(i,j) /= f;
        }
        for (j = 1; j <= k; j++) {
          AA(j,i) *= f;
        }
      }
    }
  }

  if (k-1 >= l+1) {
    /*
     * Transfer A to a Hessenberg form.
     */
    for (n = l+1; n <= k-1; n++) {
      h       = 0.;
      WK(n+m) = 0.;
      scale   = 0.;
      /*
       * Scale column
       */
      for (i = n; i <= k; i++) {
        scale += fabs(AA(i,n-1));
      }
      if (scale != 0.) {
        for (i = k; i >= n; i--) {
          WK(i+m)  = AA(i,n-1)/scale;
          h       += SQR(WK(i+m));
        }
        g        = -F77_SIGN(sqrt(h),WK(n+m));
        h       -= WK(n+m)*g;
        WK(n+m) -= g;
        /*
         * Form (I-(U*UT)/H)*A
         */
        for (j = n; j <= m; j++) {
          f = 0.;
          for (i = k; i >= n; i--) {
            f += WK(i+m)*AA(i,j);
          }
          for (i = n; i <= k; i++) {
            AA(i,j) -= WK(i+m)*f/h;
          }
        }
        /*
         * Form (i-(u*ut)/h)*a*(i-(u*ut)/h)
         */
        for (i = 1; i <= k; i++) {
          f = 0.;
          for (j = k; j >= n; j--) {
            f += WK(j+m)*AA(i,j);
          }
          for (j = n; j <= k; j++) {
            AA(i,j) -= WK(j+m)*f/h;
          }
        }
        WK(n+m)   *= scale;
        AA(n,n-1)  = scale*g;
      }
    }

    for (n = k-2; n >= l; n--) {
      n1 = n+1;
      n2 = n+2;
      f = AA(n+1,n);
      if( f != 0.) {
        f *= WK(n+1+m);
        for (i = n+2; i <= k; i++) {
          WK(i+m) = AA(i,n);
        }
        if (n+1 <= k) {
          for (j = 1; j <= m; j++) {
            g = 0.;
            for (i = n+1; i <= k; i++) {
              g += WK(i+m)*EVEC(i,j);
            }
            g /= f;
            for (i = n+1; i <= k; i++) {
              EVEC(i,j) += g*WK(i+m);
            }
          }
        }
      }
    }
  }

  n = 1;
  for (i = 1; i <= m; i++) {
    for (j = n; j <= m; j++) {
      rnorm += fabs(AA(i,j));
    }
    n = i;
    if (i < l || i > k) {
      EVAL(i) = AA(i,i);
    }
  }

  n = k;
  t = 0.;
  /*
   * Search for next eigenvalues
   */

S400:

  if (n < l) {
    goto S550;
  }

  in = 0;
  n1 = n-1;
  n2 = n-2;

  /*
   * Look for single small sub-diagonal element
   */

S410:

  for (i = l; i <= n; i++) {
    lb = n+l-i;
    if (lb == l) {
      break;
    }
    s = fabs(AA(lb-1,lb-1))+fabs(AA(lb,lb));
    if (s == 0.) {
      s = rnorm;
    }
    if (fabs(AA(lb,lb-1)) <= tol*s) {
      break;
    }
  }

  x = AA(n,n);
  if (lb == n) {
    /*
     * One eigenvalue found
     */
    AA(n,n) = x+t;
    EVAL(n) = AA(n,n);
    n       = n1;
    goto S400;
  }

  y = AA(n1,n1);
  w = AA(n,n1)*AA(n1,n);

  if (lb == n1) {
    /*
     * Two eigenvalues found
     */
    p         = (y-x)*c2;
    q         = p*p+w;
    z         = sqrt(fabs(q));
    AA(n,n)   = x+t;
    x         = AA(n,n);
    AA(n1,n1) = y+t;
    /*
     * Real pair
     */
    z        = p+F77_SIGN(z,p);
    EVAL(n1) = x+z;
    EVAL(n)  = EVAL(n1);

    if (z != 0.) {
      EVAL(n) = x-w/z;
    }
    x = AA(n,n1);
    /*
     * Employ scale factor in case X and Z are very small
     */
    r = sqrt(x*x+z*z);
    p = x/r;
    q = z/r;
    /*
     * Row modification
     */
    for (j = n1; j <= m; j++) {
      z        = AA(n1,j);
      AA(n1,j) =  q*z+p*AA(n,j);
      AA(n, j) = -p*z+q*AA(n,j);
    }
    /*
     * Column modification
     */
    for (i = 1; i <= n; i++) {
      z        = AA(i,n1);
      AA(i,n1) =  q*z+p*AA(i,n);
      AA(i,n ) = -p*z+q*AA(i,n);
    }
    /*
     * Accumulate transformations
     */
    for (i = l; i <= k; i++) {
      z          = EVEC(i,n1);
      EVEC(i,n1) =  q*z+p*EVEC(i,n);
      EVEC(i,n ) = -p*z+q*EVEC(i,n);
    }
    n = n2;
    goto S400;
  }

  if (in == 30) {
    /*
     * No convergence after 30 iterations; set error indicator to
     * the index of the current eigenvalue, and return.
     */
    *ier = n;
    return;
  }

  /*
   * Form shift
   */
  if (in == 10 || in == 20) {
    t += x;
    for (i = l; i <= n; i++) {
      AA(i,i) -= x;
    }
    s = fabs(AA(n,n1))+fabs(AA(n1,n2));
    x = c3*s;
    y = x;
    w = -c1*s*s;
  }

  in++;

  /*
   * Look for two consecutive small sub-diagonal elements
   */
  for (j = lb; j <= n2; j++) {
    i  = n2+lb-j;
    z  = AA(i,i);
    r  = x-z;
    s  = y-z;
    p  = (r*s-w)/AA(i+1,i)+AA(i,i+1);
    q  = AA(i+1,i+1)-z-r-s;
    r  = AA(i+2,i+1);
    s  = fabs(p)+fabs(q)+fabs(r);
    p /= s;
    q /= s;
    r /= s;

    if (i == lb) {
      break;
    }

    uu = fabs(AA(i,i-1))*(fabs(q)+fabs(r));
    vv = fabs(p)*(fabs(AA(i-1,i-1))+fabs(z)+fabs(AA(i+1,i+1)));

    if (uu <= tol*vv) {
      break;
    }
  }

  AA(i+2,i) = 0.;
  for (j = i+3; j <= n; j++) {
    AA(j,j-2) = 0.;
    AA(j,j-3) = 0.;
  }

  /*
   * Double QR step involving rows K to N and columns M to N
   */
  for (ka = i; ka <= n1; ka++) {
    notlas = (ka != n1);
    if (ka == i) {
      s = F77_SIGN(sqrt(p*p+q*q+r*r),p);
      if (lb != i) {
        AA(ka,ka-1) *= -1;
      }
    }
    else {
      p = AA(ka,  ka-1);
      q = AA(ka+1,ka-1);
      r = 0.;
      if (notlas) {
        r = AA(ka+2,ka-1);
      }
      x = fabs(p)+fabs(q)+fabs(r);
      if (x == 0.) {
        continue;
      }
      p /= x;
      q /= x;
      r /= x;
      s  = F77_SIGN(sqrt(p*p+q*q+r*r),p);

      AA(ka,ka-1) = -s*x;
    }

    p += s;
    x  = p/s;
    y  = q/s;
    z  = r/s;
    q /= p;
    r /= p;

    /*
     * Row modification
     */
    for (j = ka; j <= m; j++) {
      p = AA(ka,j)+q*AA(ka+1,j);
      if (notlas) {
        p          += r*AA(ka+2,j);
        AA(ka+2,j) -= p*z;
      }
      AA(ka+1,j) -= p*y;
      AA(ka,  j) -= p*x;
    }

    /*
     * Column modification
     */
    for (ii = 1; ii <= IMIN(n,ka+3); ii++) {
      p = x*AA(ii,ka)+y*AA(ii,ka+1);
      if (notlas) {
        p           += z*AA(ii,ka+2);
        AA(ii,ka+2) -= p*r;
      }
      AA(ii,ka+1) -= p*q;
      AA(ii,ka  ) -= p;
    }

    /*
     * Accumulate transformations
     */
    for (ii = l; ii <= k; ii++) {
      p = x*EVEC(ii,ka)+y*EVEC(ii,ka+1);
      if (notlas) {
        p             += z*EVEC(ii,ka+2);
        EVEC(ii,ka+2) -= p*r;
      }
      EVEC(ii,ka+1) -= p*q;
      EVEC(ii,ka  ) -= p;
    }
  }

  goto S410;

  /*
   * All evals found, now backsubstitute real vector
   */

S550:

  if (rnorm != 0.) {
    for (n = m; n >= 1; n--) {
      n2      = n;
      AA(n,n) = 1.;
      for (i = n-1; i >= 1; i--) {
        w = AA(i,i)-EVAL(n);
        if (w == 0.) {
          w = tol*rnorm;
        }
        r = AA(i,n);
        for (j = n2; j <= n-1; j++) {
          r += AA(i,j)*AA(j,n);
        }
        AA(i,n) = -r/w;
        n2      = i;
      }
    }
    /*
     * End backsubstitution vectors of isolated evals
     */
    for (i = 1; i <= m; i++) {
      if (i < l || i > k) {
        for (j = i; j <= m; j++) {
          EVEC(i,j) = AA(i,j);
        }
      }
    }
    /*
     * Multiply by transformation matrix
     */
    if (k != 0) {
      for (j = m; j >= l; j--) {
        for (i = l; i <= k; i++) {
          z = 0.;
          for (n = l; n <= IMIN(j,k); n++) {
            z += EVEC(i,n)*AA(n,j);
          }
          EVEC(i,j) = z;
        }
      }
    }
  }
  for (i = l; i <= k; i++) {
    for (j = 1; j <= m; j++) {
      EVEC(i,j) *= WK(i);
    }
  }

  /*
   * Interchange rows if permutations occurred
   */
  for (i = l-1; i >= 1; i--) {
    j = WK(i);
    if (i != j) {
      for (n = 1; n <= m; n++) {
        repl      = EVEC(i,n);
        EVEC(i,n) = EVEC(j,n);
        EVEC(j,n) = repl;
      }
    }
  }
  for (i = k+1; i <= m; i++) {
    j = WK(i);
    if (i != j) {
      for (n = 1; n <= m; n++) {
        repl      = EVEC(i,n);
        EVEC(i,n) = EVEC(j,n);
        EVEC(j,n) = repl;
      }
    }
  }

  return;
}

/*============================= end of c_asymmetric_matrix() ============*/

/*============================= c_solve_eigen() =========================*/

/*
   Solves eigenvalue/vector problem necessary to construct homogeneous
   part of discrete ordinate solution; STWJ(8b), STWL(23f)
   ** NOTE ** Eigenvalue problem is degenerate when single scattering
              albedo = 1;  present way of doing it seems numerically more
              stable than alternative methods that we tried

   I N P U T     V A R I A B L E S:

       ds     :  Disort state variables
       lc     :
       gl     :  Delta-M scaled Legendre coefficients of phase function
                 (including factors 2l+1 and single-scatter albedo)
       cmu    :  Computational polar angle cosines
       cwt    :  Weights for quadrature over polar angle cosine
       mazim  :  Order of azimuthal component
       nn     :  Half the total number of streams
       ylmc   :  Normalized associated Legendre polynomial
                 at the quadrature angles CMU


   O U T P U T    V A R I A B L E S:

       cc     :  C-sub-ij in eq. SS(5); needed in SS(15&18)
       eval   :  NN eigenvalues of eq. SS(12), STWL(23f) on return
                 from asymmetric_matrix but then square roots taken
       evecc  :  NN eigenvectors  (G+) - (G-)  on return
                 from asymmetric_matrix ( column j corresponds to EVAL(j) )
                 but then  (G+) + (G-)  is calculated from SS(10),
                 G+  and  G-  are separated, and  G+  is stacked on
                 top of  G-  to form NSTR eigenvectors of SS(7)
       gc     :  Permanent storage for all NSTR eigenvectors, but
                 in an order corresponding to KK
       kk     :  Permanent storage for all NSTR eigenvalues of SS(7),
                 but re-ordered with negative values first ( square
                 roots of EVAL taken and negatives added )


   I N T E R N A L   V A R I A B L E S:

       ab            :  Matrices AMB (alpha-beta), APB (alpha+beta) in reduced eigenvalue problem (see cdisort.h)
       array         :  Complete coefficient matrix of reduced eigenvalue
                        problem: (alpha+beta)*(alpha-beta)
       gpplgm        :  (g+) + (g-) (cf. eqs. SS(10-11))
       gpmigm        :  (g+) - (g-) (cf. eqs. SS(10-11))
       wk            :  Scratch array required by asymmetric_matrix

   Called by- c_disort, c_albtrans
   Calls- c_asymmetric_matrix, c_errmsg
 -------------------------------------------------------------------*/

/*
 * NOTE: Here the scratch array ARRAY(,) is half the size in each dimension compared to other subroutines
 */
#undef  ARRAY
#define ARRAY(iq,jq) array[iq-1+(jq-1)*(ds->nstr/2)]

DISPATCH_MACRO inline void c_solve_eigen(disort_state *ds,
                   int           lc,
                   disort_pair  *ab,
                   double       *array,
                   double       *cmu,
                   double       *cwt,
                   double       *gl,
                   int           mazim,
                   int           nn,
                   double       *ylmc,
                   double       *cc,
                   double       *evecc,
                   double       *eval,
                   double       *kk,
                   double       *gc,
                   double       *wk)
{
  int
    ier;
  int
    iq,jq,kq,l;
  double
    alpha,beta,gpmigm,gpplgm,sum;

  /*
   * Calculate quantities in eqs. SS(5-6), STWL(8b,15,23f)
   */
  for (iq = 1; iq <= nn; iq++) {
    for (jq = 1; jq <= ds->nstr; jq++) {
      sum = 0.;
      for (l = mazim; l <= ds->nstr-1; l++) {
        sum += GL(l,lc)*YLMC(l,iq)*YLMC(l,jq);
      }
      CC(iq,jq) = .5*sum*CWT(jq);
    }
    for (jq = 1; jq <= nn; jq++) {
      /*
       * Fill remainder of array using symmetry relations  C(-mui,muj) = C(mui,-muj) and C(-mui,-muj) = C(mui,muj)
       */
      CC(iq+nn,jq   ) = CC(iq,jq+nn);
      CC(iq+nn,jq+nn) = CC(iq,jq   );
      /*
       * Get factors of coeff. matrix of reduced eigenvalue problem
       */
      alpha      = CC(iq,jq   )/CMU(iq);
      beta       = CC(iq,jq+nn)/CMU(iq);
      AMB(iq,jq) = alpha-beta;
      APB(iq,jq) = alpha+beta;
    }
    AMB(iq,iq) -= 1./CMU(iq);
    APB(iq,iq) -= 1./CMU(iq);
  }
  /*
   * Finish calculation of coefficient matrix of reduced eigenvalue problem: 
   * get matrix product (alpha+beta)*(alpha-beta); SS(12),STWL(23f)
   */
  for (iq = 1; iq <= nn; iq++) {
    for (jq = 1; jq <= nn; jq++) {
      sum = 0.;
      for (kq = 1; kq <= nn; kq++) {
        sum += APB(iq,kq)*AMB(kq,jq);
      }
      ARRAY(iq,jq) = sum;
    }
  }

  /*
   * Find (real) eigenvalues and eigenvectors
   */
  c_asymmetric_matrix(array,evecc,eval,nn,ds->nstr/2,ds->nstr,&ier,wk);

  if (ier > 0) {
    fprintf(stderr,"\n\n asymmetric_matrix--eigenvalue no. %4d didn't converge.  Lower-numbered eigenvalues wrong.\n",ier);
    c_errmsg("asymmetric_matrix--convergence problems",DS_ERROR);
  }

  for (iq = 1; iq <= nn; iq++) {
    EVAL(iq)     = sqrt(fabs(EVAL(iq)));
    KK(iq+nn,lc) = EVAL(iq);
    /*
     * Add negative eigenvalue
     */
    KK(nn+1-iq,lc) = -EVAL(iq);
  }

  /*
   * Find eigenvectors (G+) + (G-) from SS(10) and store temporarily in APB array
   */
  for (jq = 1; jq <= nn; jq++) {
    for (iq = 1; iq <= nn; iq++) {
      sum = 0.;
      for (kq = 1; kq <= nn; kq++) {
        sum += AMB(iq,kq)*EVECC(kq,jq);
      }
      APB(iq,jq) = sum/EVAL(jq);
    }
  }
  for (jq = 1; jq <= nn; jq++) {
    for (iq = 1; iq <= nn; iq++) {
      gpplgm = APB(  iq,jq);
      gpmigm = EVECC(iq,jq);
      /*
       * Recover eigenvectors G+,G- from their sum and difference; stack them to get eigenvectors of full system
       * SS(7) (JQ = eigenvector number)
       */
      EVECC(iq,   jq) = .5*(gpplgm+gpmigm);
      EVECC(iq+nn,jq) = .5*(gpplgm-gpmigm);
      /*
       * Eigenvectors corresponding to negative eigenvalues (corresp. to reversing sign of 'k' in SS(10) )
       */
      gpplgm *= -1; 
      EVECC(iq,   jq+nn)     = .5*(gpplgm+gpmigm);
      EVECC(iq+nn,jq+nn)     = .5*(gpplgm-gpmigm);
      GC(nn+iq,  nn+jq,  lc) = EVECC(iq,   jq   );
      GC(nn-iq+1,nn+jq,  lc) = EVECC(iq+nn,jq   );
      GC(nn+iq,  nn-jq+1,lc) = EVECC(iq,   jq+nn);
      GC(nn-iq+1,nn-jq+1,lc) = EVECC(iq+nn,jq+nn);
    }
  }

  return;
}

/*============================= end of c_solve_eigen() ==================*/

DISPATCH_MACRO inline void c_solve_eigen4(disort_state *ds,
                                          int lc,
                                          disort_pair *ab,
                                          double *array,
                                          double *cmu,
                                          double *cwt,
                                          double *gl,
                                          double *ylmc,
                                          double *cc,
                                          double *evecc,
                                          double *eval,
                                          double *kk,
                                          double *gc,
                                          double *wk)
{
  int ier;
  for (int iq = 0; iq < 2; ++iq) {
    for (int jq = 0; jq < 4; ++jq) {
      double sum = 0.;
      for (int l = 0; l < 4; ++l) {
        sum += gl[l + (lc - 1) * 5] * ylmc[l + iq * 5] *
               ylmc[l + jq * 5];
      }
      cc[iq + jq * 4] = .5 * sum * cwt[jq];
    }
    for (int jq = 0; jq < 2; ++jq) {
      cc[iq + 2 + jq * 4] = cc[iq + (jq + 2) * 4];
      cc[iq + 2 + (jq + 2) * 4] = cc[iq + jq * 4];
      double alpha = cc[iq + jq * 4] / cmu[iq];
      double beta = cc[iq + (jq + 2) * 4] / cmu[iq];
      ab[iq + jq * 2].zero = alpha - beta;
      ab[iq + jq * 2].one = alpha + beta;
    }
    ab[iq + iq * 2].zero -= 1. / cmu[iq];
    ab[iq + iq * 2].one -= 1. / cmu[iq];
  }

  for (int iq = 0; iq < 2; ++iq) {
    for (int jq = 0; jq < 2; ++jq) {
      double sum = 0.;
      for (int kq = 0; kq < 2; ++kq) {
        sum += ab[iq + kq * 2].one * ab[kq + jq * 2].zero;
      }
      array[iq + jq * 2] = sum;
    }
  }

  c_asymmetric_matrix(array, evecc, eval, 2, 2, 4, &ier, wk);
  if (ier > 0) {
    fprintf(stderr,
            "\n\n asymmetric_matrix--eigenvalue no. %4d didn't converge.  "
            "Lower-numbered eigenvalues wrong.\n",
            ier);
    c_errmsg("asymmetric_matrix--convergence problems", DS_ERROR);
  }

  for (int iq = 0; iq < 2; ++iq) {
    eval[iq] = sqrt(fabs(eval[iq]));
    kk[iq + 2 + (lc - 1) * 4] = eval[iq];
    kk[1 - iq + (lc - 1) * 4] = -eval[iq];
  }

  for (int jq = 0; jq < 2; ++jq) {
    for (int iq = 0; iq < 2; ++iq) {
      double sum = 0.;
      for (int kq = 0; kq < 2; ++kq) {
        sum += ab[iq + kq * 2].zero * evecc[kq + jq * 4];
      }
      ab[iq + jq * 2].one = sum / eval[jq];
    }
  }

  for (int jq = 0; jq < 2; ++jq) {
    for (int iq = 0; iq < 2; ++iq) {
      double gpplgm = ab[iq + jq * 2].one;
      double gpmigm = evecc[iq + jq * 4];
      evecc[iq + jq * 4] = .5 * (gpplgm + gpmigm);
      evecc[iq + 2 + jq * 4] = .5 * (gpplgm - gpmigm);
      gpplgm *= -1.;
      evecc[iq + (jq + 2) * 4] = .5 * (gpplgm + gpmigm);
      evecc[iq + 2 + (jq + 2) * 4] = .5 * (gpplgm - gpmigm);
      gc[2 + iq + (2 + jq + (lc - 1) * 4) * 4] =
          evecc[iq + jq * 4];
      gc[1 - iq + (2 + jq + (lc - 1) * 4) * 4] =
          evecc[iq + 2 + jq * 4];
      gc[2 + iq + (1 - jq + (lc - 1) * 4) * 4] =
          evecc[iq + (jq + 2) * 4];
      gc[1 - iq + (1 - jq + (lc - 1) * 4) * 4] =
          evecc[iq + 2 + (jq + 2) * 4];
    }
  }
}

DISPATCH_MACRO inline void c_solve_eigen8(disort_state *ds,
                                          int lc,
                                          disort_pair *ab,
                                          double *array,
                                          double *cmu,
                                          double *cwt,
                                          double *gl,
                                          double *ylmc,
                                          double *cc,
                                          double *evecc,
                                          double *eval,
                                          double *kk,
                                          double *gc,
                                          double *wk)
{
  int ier;
  for (int iq = 0; iq < 4; ++iq) {
    for (int jq = 0; jq < 8; ++jq) {
      double sum = 0.;
      for (int l = 0; l < 8; ++l) {
        sum += gl[l + (lc - 1) * 9] * ylmc[l + iq * 9] *
               ylmc[l + jq * 9];
      }
      cc[iq + jq * 8] = .5 * sum * cwt[jq];
    }
    for (int jq = 0; jq < 4; ++jq) {
      cc[iq + 4 + jq * 8] = cc[iq + (jq + 4) * 8];
      cc[iq + 4 + (jq + 4) * 8] = cc[iq + jq * 8];
      double alpha = cc[iq + jq * 8] / cmu[iq];
      double beta = cc[iq + (jq + 4) * 8] / cmu[iq];
      ab[iq + jq * 4].zero = alpha - beta;
      ab[iq + jq * 4].one = alpha + beta;
    }
    ab[iq + iq * 4].zero -= 1. / cmu[iq];
    ab[iq + iq * 4].one -= 1. / cmu[iq];
  }

  for (int iq = 0; iq < 4; ++iq) {
    for (int jq = 0; jq < 4; ++jq) {
      double sum = 0.;
      for (int kq = 0; kq < 4; ++kq) {
        sum += ab[iq + kq * 4].one * ab[kq + jq * 4].zero;
      }
      array[iq + jq * 4] = sum;
    }
  }

  c_asymmetric_matrix(array, evecc, eval, 4, 4, 8, &ier, wk);
  if (ier > 0) {
    fprintf(stderr,
            "\n\n asymmetric_matrix--eigenvalue no. %4d didn't converge.  "
            "Lower-numbered eigenvalues wrong.\n",
            ier);
    c_errmsg("asymmetric_matrix--convergence problems", DS_ERROR);
  }

  for (int iq = 0; iq < 4; ++iq) {
    eval[iq] = sqrt(fabs(eval[iq]));
    kk[iq + 4 + (lc - 1) * 8] = eval[iq];
    kk[3 - iq + (lc - 1) * 8] = -eval[iq];
  }

  for (int jq = 0; jq < 4; ++jq) {
    for (int iq = 0; iq < 4; ++iq) {
      double sum = 0.;
      for (int kq = 0; kq < 4; ++kq) {
        sum += ab[iq + kq * 4].zero * evecc[kq + jq * 8];
      }
      ab[iq + jq * 4].one = sum / eval[jq];
    }
  }
  for (int jq = 0; jq < 4; ++jq) {
    for (int iq = 0; iq < 4; ++iq) {
      double gpplgm = ab[iq + jq * 4].one;
      double gpmigm = evecc[iq + jq * 8];
      evecc[iq + jq * 8] = .5 * (gpplgm + gpmigm);
      evecc[iq + 4 + jq * 8] = .5 * (gpplgm - gpmigm);
      gpplgm *= -1.;
      evecc[iq + (jq + 4) * 8] = .5 * (gpplgm + gpmigm);
      evecc[iq + 4 + (jq + 4) * 8] = .5 * (gpplgm - gpmigm);
      gc[4 + iq + (4 + jq + (lc - 1) * 8) * 8] =
          evecc[iq + jq * 8];
      gc[3 - iq + (4 + jq + (lc - 1) * 8) * 8] =
          evecc[iq + 4 + jq * 8];
      gc[4 + iq + (3 - jq + (lc - 1) * 8) * 8] =
          evecc[iq + (jq + 4) * 8];
      gc[3 - iq + (3 - jq + (lc - 1) * 8) * 8] =
          evecc[iq + 4 + (jq + 4) * 8];
    }
  }
}

/* The nstr=8 flux-only boundary system is block tridiagonal by layer. */
DISPATCH_MACRO inline int c_solve_block_tridiag8(double const *cband,
                                                   int nblock, double *b,
                                                   int *ipvt,
                                                   double *work)
{
  double *diag = work;
  double *lower = diag + nblock * 64;
  double *upper = lower + (nblock - 1) * 64;
  const int lda = 34;
  const int middle = 23;

  for (int block = 0; block < nblock; ++block) {
    double *d = diag + block * 64;
    for (int j = 0; j < 8; ++j) {
      int col = block * 8 + j + 1;
      for (int i = 0; i < 8; ++i) {
        int row = block * 8 + i + 1;
        int band_row = row - col + middle;
        d[i + j * 8] = (band_row >= 1 && band_row <= lda)
                           ? cband[(band_row - 1) + (col - 1) * lda]
                           : 0.;
      }
    }
    if (block > 0) {
      double *l = lower + (block - 1) * 64;
      for (int j = 0; j < 8; ++j) {
        int col = (block - 1) * 8 + j + 1;
        for (int i = 0; i < 8; ++i) {
          int row = block * 8 + i + 1;
          int band_row = row - col + middle;
          l[i + j * 8] = (band_row >= 1 && band_row <= lda)
                             ? cband[(band_row - 1) + (col - 1) * lda]
                             : 0.;
        }
      }
    }
    if (block + 1 < nblock) {
      double *u = upper + block * 64;
      for (int j = 0; j < 8; ++j) {
        int col = (block + 1) * 8 + j + 1;
        for (int i = 0; i < 8; ++i) {
          int row = block * 8 + i + 1;
          int band_row = row - col + middle;
          u[i + j * 8] = (band_row >= 1 && band_row <= lda)
                             ? cband[(band_row - 1) + (col - 1) * lda]
                             : 0.;
        }
      }
    }
  }

  for (int block = 0; block + 1 < nblock; ++block) {
    double *d = diag + block * 64;
    double *u = upper + block * 64;
    double *next_d = diag + (block + 1) * 64;
    double *l = lower + block * 64;
    double *rhs = b + block * 8;
    double *next_rhs = rhs + 8;
    int info;
    c_sgefa(d, 8, 8, ipvt + block * 8, &info);
    if (info != 0) return FALSE;
    c_sgesl(d, 8, 8, ipvt + block * 8, rhs, 0);
    for (int j = 0; j < 8; ++j) {
      c_sgesl(d, 8, 8, ipvt + block * 8, u + j * 8, 0);
    }

    for (int j = 0; j < 8; ++j) {
      for (int i = 0; i < 8; ++i) {
        double value = 0.;
        for (int k = 0; k < 8; ++k) value += l[i + k * 8] * u[k + j * 8];
        next_d[i + j * 8] -= value;
      }
    }
    for (int i = 0; i < 8; ++i) {
      double value = 0.;
      for (int k = 0; k < 8; ++k) value += l[i + k * 8] * rhs[k];
      next_rhs[i] -= value;
    }
  }

  int last = nblock - 1;
  int info;
  c_sgefa(diag + last * 64, 8, 8, ipvt + last * 8, &info);
  if (info != 0) return FALSE;
  c_sgesl(diag + last * 64, 8, 8, ipvt + last * 8, b + last * 8, 0);

  for (int block = nblock - 2; block >= 0; --block) {
    double *rhs = b + block * 8;
    double const *u = upper + block * 64;
    double const *next_rhs = rhs + 8;
    for (int i = 0; i < 8; ++i) {
      double value = 0.;
      for (int j = 0; j < 8; ++j) value += u[i + j * 8] * next_rhs[j];
      rhs[i] -= value;
    }
  }

  return TRUE;
}

/* Solve an nstr=8 block-tridiagonal system already stored as contiguous
 * diagonal, lower, and upper eight-by-eight blocks. */
DISPATCH_MACRO inline int c_solve_block_tridiag8_blocks(int nblock,
                                                          double *b,
                                                          int *ipvt,
                                                          double *work)
{
  double *diag = work;
  double *lower = diag + nblock * 64;
  double *upper = lower + (nblock - 1) * 64;

  for (int block = 0; block + 1 < nblock; ++block) {
    double *d = diag + block * 64;
    double *u = upper + block * 64;
    double *next_d = diag + (block + 1) * 64;
    double *l = lower + block * 64;
    double *rhs = b + block * 8;
    double *next_rhs = rhs + 8;
    int info;
    c_sgefa(d, 8, 8, ipvt + block * 8, &info);
    if (info != 0) return FALSE;
    c_sgesl(d, 8, 8, ipvt + block * 8, rhs, 0);
    for (int j = 0; j < 8; ++j) {
      c_sgesl(d, 8, 8, ipvt + block * 8, u + j * 8, 0);
    }

    for (int j = 0; j < 8; ++j) {
      for (int i = 0; i < 8; ++i) {
        double value = 0.;
        for (int k = 0; k < 8; ++k) value += l[i + k * 8] * u[k + j * 8];
        next_d[i + j * 8] -= value;
      }
    }
    for (int i = 0; i < 8; ++i) {
      double value = 0.;
      for (int k = 0; k < 8; ++k) value += l[i + k * 8] * rhs[k];
      next_rhs[i] -= value;
    }
  }

  int last = nblock - 1;
  int info;
  c_sgefa(diag + last * 64, 8, 8, ipvt + last * 8, &info);
  if (info != 0) return FALSE;
  c_sgesl(diag + last * 64, 8, 8, ipvt + last * 8, b + last * 8, 0);

  for (int block = nblock - 2; block >= 0; --block) {
    double *rhs = b + block * 8;
    double const *u = upper + block * 64;
    double const *next_rhs = rhs + 8;
    for (int i = 0; i < 8; ++i) {
      double value = 0.;
      for (int j = 0; j < 8; ++j) value += u[i + j * 8] * next_rhs[j];
      rhs[i] -= value;
    }
  }

  return TRUE;
}

/*============================= c_solve0() ==============================*/

/*
        Construct right-hand side vector B for general boundary
        conditions STWJ(17) and solve system of equations obtained
        from the boundary conditions and the continuity-of-
        intensity-at-layer-interface equations.
        Thermal emission contributes only in azimuthal independence.

    I N P U T      V A R I A B L E S:

       ds       :  Disort input variables
       bdr      :  Surface bidirectional reflectivity
       bem      :  Surface bidirectional emissivity
       bplanck  :  Bottom boundary thermal emission
       cband    :  Left-hand side matrix of linear system eq. SC(5),
                   scaled by eq. SC(12); in banded form required
                   by LINPACK solution routines
       cmu,cwt  :  Abscissae, weights for Gauss quadrature
                   over angle cosine
       expbea   :  Transmission of incident beam, EXP(-TAUCPR/UMU0)
       lyrcut   :  Logical flag for truncation of computational layers
       mazim    :  Order of azimuthal component
       ncol     :  Number of columns in CBAND
       nn       :  Order of double-Gauss quadrature (NSTR/2)
       ncut     :  Total number of computational layers considered
       tplanck  :  Top boundary thermal emission
       taucpr   :  Cumulative optical depth (delta-M-scaled)
       zz       :  Beam source vectors in eq. SS(19), STWL(24b)
       zzg      :  Beam source vectors in eq. KS(10)for a general source constant over a layer
       plk      :  Thermal source vectors z0,z1 by solving eq. SS(16), Y0,Y1 in STWL(26b,a);
                   plk[].zero, plk[].one (see cdisort.h)

    O U T P U T     V A R I A B L E S:

       b        :  Right-hand side vector of eq. SC(5) going into
                   sgbsl; returns as solution vector of eq. SC(12),
                   constants of integration without exponential term
      ll        :  Permanent storage for B, but re-ordered

   I N T E R N A L    V A R I A B L E S:

       ipvt     :  Integer vector of pivot indices
       it       :  Pointer for position in  B
       ncd      :  Number of diagonals below or above main diagonal
       rcond    :  Indicator of singularity for cband
       z        :  Scratch array required by sgbco

   Called by- c_disort
   Calls- c_sgbco, c_errmsg, c_sgbsl
 +-------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_solve0(disort_state *ds,
              double       *b,
              double       *bdr,
              double       *bem,
              double        bplanck,
              double       *cband,
              double       *cmu,
              double       *cwt,
              double       *expbea,
              int          *ipvt,
              double       *ll,
              int           lyrcut,
              int           mazim,
              int           ncol,
              int           ncut,
              int           nn,
              double        tplanck,
              double       *taucpr,
              double       *z,
              disort_pair  *zbeamsp,
	      double       *zbeama,
              double       *zz,
              double       *zzg,
              disort_pair  *plk)
{
  int
    ipnt,iq,it,jq,lc,ncd,solved_by_blocks;
  double sum,diff;
#ifndef __CUDA_ARCH__
  double rcond;
#endif
  
  memset(b,0,ds->nstr*ds->nlyr*sizeof(double));
  solved_by_blocks = FALSE;
  
  /*
   * Construct B, STWJ(20a,c) for parallel beam+bottom
   * reflection+thermal emission at top and/or bottom
   */
  if (mazim > 0 && ( ds->bc.fbeam > 0.  || ds->flag.general_source) ) {
    /*
     * Azimuth-dependent case (never called if FBEAM = 0)
     */
    if ( lyrcut == TRUE || ds->flag.lamber == TRUE ) {
      /*
       * No azimuthal-dependent intensity for Lambert surface; no
       * intensity component for truncated bottom layer
       */
      for (iq = 1; iq <= nn; iq++) {
        /*
         * Top boundary
         */
	if ( ds->flag.spher == TRUE ) {
	  B(iq) = - ZBEAM0(nn+1-iq,1);
	}
	else {
	  B(iq) = - ZZ(nn+1-iq,1);
	}
	if ( ds->flag.general_source == TRUE ) {
	  B(iq) -= ZZG(nn+1-iq,1);
	  //aky	  B(iq) = B(iq) - ZZG(nn+1-iq,1);
	}
        /*
         * Bottom boundary
         */
	if ( ds->flag.spher == TRUE ) {
	  B(ncol-nn+iq) = - exp(-ZBEAMA(ncut)*TAUCPR(ncut))*
	    (ZBEAM0(iq+nn,ncut) + ZBEAM1(iq+nn,ncut)*TAUCPR(ncut));
	}
	else {
	  B(ncol-nn+iq) = - ZZ(iq+nn,ncut)*EXPBEA(ncut);
	}
	if ( ds->flag.general_source == TRUE ) {
	  B(ncol-nn+iq) -=  ZZG(iq+nn,ncut);
	  //aky	  B(ncol-nn+iq) = B(ncol-nn+iq)  - ZZG(iq+nn,ncut);
	}
      }
    }
    else {
      for (iq = 1; iq <= nn; iq++) {
	if ( ds->flag.spher == TRUE ) {
	  B(iq) = - ZBEAM0(nn+1-iq,1);
	}
	else {
	  B(iq) = - ZZ(nn+1-iq,1);
	}
	if ( ds->flag.general_source == TRUE ) {
	  B(iq) -= ZZG(nn+1-iq,1);
	  //aky	  B(iq) = B(iq) - ZZG(nn+1-iq,1);
	}
	if ( ds->flag.spher == TRUE ) {
	  c_errmsg("solve0--BDR not implemented for pseudo-spherical geometry",
		   DS_WARNING);
	}
	else {
	  sum   = 0.;
	  for (jq = 1; jq <= nn; jq++) {
	    sum += CWT(jq)*CMU(jq)*BDR(iq,jq)*ZZ(nn+1-jq,ncut)*EXPBEA(ncut);
	  }
	  B(ncol-nn+iq) = sum;
	  if ( ds->flag.general_source == TRUE ) {
	    sum   = 0.;
	    for (jq = 1; jq <= nn; jq++) {
	      sum += CWT(jq)*CMU(jq)*BDR(iq,jq)*ZZG(nn+1-jq,ncut);
	    }
	    B(ncol-nn+iq) += sum;
	  }
	}
        if (ds->bc.fbeam > 0.) {
	  if ( ds->flag.spher == TRUE ) {
	    c_errmsg("solve0--BDR not implemented for pseudo-spherical geometry",
		     DS_WARNING)  ;
	  }
	  else {
	    B(ncol-nn+iq) += (BDR(iq,0)*ds->bc.umu0*ds->bc.fbeam/
			      M_PI-ZZ(iq+nn,ncut))*EXPBEA(ncut);
	  }
        }
	if ( ds->flag.general_source == TRUE ) {
	    B(ncol-nn+iq) += -ZZG(iq+nn,ncut);
	}
      }
    }
    /*
     * Continuity condition for layer interfaces of eq. STWJ(20b)
     */
    it = nn;
    diff = 0;
    for (lc = 1; lc <= ncut-1; lc++) {
      for (iq = 1; iq <= ds->nstr; iq++) {
	if ( ds->flag.general_source == TRUE ) {
	  diff = (ZZG(iq,lc+1)-ZZG(iq,lc));
	}
	if ( ds->flag.spher == TRUE ) {
	  B(++it) = exp(-ZBEAMA(lc+1)*TAUCPR(lc))*
	    (ZBEAM0(iq,lc+1)+ZBEAM1(iq,lc+1)*TAUCPR(lc))
	    -  exp(-ZBEAMA(lc)*TAUCPR(lc))*
	    (ZBEAM0(iq,lc)+ZBEAM1(iq,lc)*TAUCPR(lc))
	    + diff;
	}
	else {
	  B(++it) = (ZZ(iq,lc+1)-ZZ(iq,lc))*EXPBEA(lc)  + diff;
	}
      }
    }
  }
  else {
    /*
     * Azimuth-independent case
     */
    if (ds->bc.fbeam == 0. && ds->flag.general_source == FALSE ) {
      for (iq = 1; iq <= nn; iq++) {
        /*
         * Top boundary
         */
        B(iq) = -ZPLK0(nn+1-iq,1)+ds->bc.fisot+tplanck;
      }
      if ( lyrcut == TRUE ) {
        /*
         * No intensity component for truncated bottom layer
         */
        for (iq = 1; iq <= nn; iq++) {
          /*
           * Bottom boundary
           */
          B(ncol-nn+iq) = -ZPLK0(iq+nn,ncut)-ZPLK1(iq+nn,ncut)*TAUCPR(ncut);
        }
      }
      else {
        for (iq = 1; iq <= nn; iq++) {
          sum = 0.;
          for (jq = 1; jq <= nn; jq++) {
            sum += CWT(jq)*CMU(jq)*BDR(iq,jq)*
	      (ZPLK0(nn+1-jq,ncut)+ZPLK1(nn+1-jq,ncut)*TAUCPR(ncut));
          }
          B(ncol-nn+iq) = 2.*sum+BEM(iq)*bplanck-
	    ZPLK0(iq+nn,ncut)-ZPLK1(iq+nn,ncut)*TAUCPR(ncut);
        }
      }
      /*
       * Continuity condition for layer interfaces, STWJ(20b)
       */
      it = nn;
      for (lc = 1; lc <= ncut-1; lc++) {
        for (iq = 1; iq <= ds->nstr; iq++) {
          B(++it) = ZPLK0(iq,lc+1)-ZPLK0(iq,lc)+
	    (ZPLK1(iq,lc+1)-ZPLK1(iq,lc))*TAUCPR(lc);
        }
      }
    }
    else {
      if ( ds->flag.spher == TRUE ) {
	for (iq = 1; iq <= nn; iq++) 
	  B(iq) = -ZBEAM0(nn+1-iq,1)-ZPLK0(nn+1-iq,1)+ds->bc.fisot+tplanck;
      }
      else {
	for (iq = 1; iq <= nn; iq++) 
	  B(iq) = -ZZ(nn+1-iq,1)-ZPLK0(nn+1-iq,1)+ds->bc.fisot+tplanck;
      }
      if ( ds->flag.general_source == TRUE ) {
	for (iq = 1; iq <= nn; iq++) 
	  B(iq) -= ZZG(nn+1-iq,1);
	//aky	  B(iq) = B(iq) - ZZG(nn+1-iq,1);
      }
      if (lyrcut) {
	if ( ds->flag.spher == TRUE ) {
	  for (iq = 1; iq <= nn; iq++) {
	    B(ncol-nn+iq) = -exp(-ZBEAMA(ncut)*TAUCPR(ncut))*
	      (ZBEAM0(iq+nn,ncut)+ ZBEAM1(iq+nn,ncut)*TAUCPR(ncut))
	      -ZPLK0(iq+nn,ncut)-ZPLK1(iq+nn,ncut)*TAUCPR(ncut);
	  }
	}
	else {
	  for (iq = 1; iq <= nn; iq++) {
	    B(ncol-nn+iq) = -ZZ(iq+nn,ncut)*EXPBEA(ncut)
	      -ZPLK0(iq+nn,ncut)-ZPLK1(iq+nn,ncut)*TAUCPR(ncut);
	  }
	}
	if ( ds->flag.general_source == TRUE ) {
	  for (iq = 1; iq <= nn; iq++) 
	    B(ncol-nn+iq) -= ZZG(iq+nn,ncut);
	  //aky	    B(ncol-nn+iq) = B(ncol-nn+iq) - ZZG(iq+nn,ncut);
	}
      }
      else {
	if ( ds->flag.spher == TRUE ) {
	  for (iq = 1; iq <= nn; iq++) {
	    sum = 0.;
	    for (jq = 1; jq <= nn; jq++) {
	      sum += CWT(jq)*CMU(jq)*BDR(iq,jq)*
		( exp(-ZBEAMA(ncut)*TAUCPR(ncut))*
		  (ZBEAM0(nn+1-jq,ncut)+ZBEAM1(nn+1-jq,ncut)*TAUCPR(ncut))
		  + ZZG(nn+1-jq,ncut)
		  + ZPLK0(nn+1-jq,ncut)+ZPLK1(nn+1-jq,ncut)*TAUCPR(ncut));
	    }
	    B(ncol-nn+iq) = 2.0*sum +
	      ( BDR(iq,0)*ds->bc.umu0*ds->bc.fbeam/M_PI) *EXPBEA(ncut)
	      -  exp(-ZBEAMA(ncut)*TAUCPR(ncut))*
	      (ZBEAM0(iq+nn,ncut)+ZBEAM1(iq+nn,ncut)*TAUCPR(ncut))
	      - ZZG(iq+nn,ncut)
	      + BEM(iq)*bplanck
	      -ZPLK0(iq+nn,ncut)-ZPLK1(iq+nn,ncut)*TAUCPR(ncut)
	      +ds->bc.fluor;
	  }
	}
	else {
	  for (iq = 1; iq <= nn; iq++) {
	    sum = 0.;
	    for (jq = 1; jq <= nn; jq++) {
	      sum += CWT(jq)*CMU(jq)*BDR(iq,jq)*
		(ZZ(nn+1-jq,ncut)*EXPBEA(ncut)+ZPLK0(nn+1-jq,ncut)
		 + ZZG(nn+1-jq,ncut)
		 +ZPLK1(nn+1-jq,ncut)*TAUCPR(ncut));
	    }
	    B(ncol-nn+iq) = 2.*sum+
	      (BDR(iq,0)*ds->bc.umu0*ds->bc.fbeam/M_PI-ZZ(iq+nn,ncut))
	      *EXPBEA(ncut)
	      - ZZG(iq+nn,ncut)
	      +BEM(iq)*bplanck-ZPLK0(iq+nn,ncut)-ZPLK1(iq+nn,ncut)*TAUCPR(ncut)
	      +ds->bc.fluor;
	  }
	}
      }
      it = nn;
      if ( ds->flag.spher == TRUE ) {
	for (lc = 1; lc <= ncut-1; lc++) {
	  for (iq = 1; iq <= ds->nstr; iq++) {
	    B(++it) = exp(-ZBEAMA(lc+1)*TAUCPR(lc))*
	      (ZBEAM0(iq,lc+1)+ZBEAM1(iq,lc+1)*TAUCPR(lc))
	      -exp(-ZBEAMA(lc)*TAUCPR(lc))*
	      (ZBEAM0(iq,lc)+ZBEAM1(iq,lc)*TAUCPR(lc))
	      +ZZG(iq,lc+1)-ZZG(iq,lc)
	      +ZPLK0(iq,lc+1)-ZPLK0(iq,lc)+
	      (ZPLK1(iq,lc+1)-ZPLK1(iq,lc))*TAUCPR(lc);
	  }
	}
      }
      else {
	for (lc = 1; lc <= ncut-1; lc++) {
	  for (iq = 1; iq <= ds->nstr; iq++) {
	    B(++it) = (ZZ(iq,lc+1)-ZZ(iq,lc))*EXPBEA(lc)
	      +ZZG(iq,lc+1)-ZZG(iq,lc)
	      +ZPLK0(iq,lc+1)-ZPLK0(iq,lc)
	      +(ZPLK1(iq,lc+1)-ZPLK1(iq,lc))*TAUCPR(lc);
	  }
	}
      }
    }
  }

  /*
   * Find the L-U decomposition of the band matrix CBAND.  The CUDA path
   * omits the condition-number estimate because it is only diagnostic.
   */
  ncd   = 3*nn-1;
  if (ds->fast_flux && ds->nstr == 8 && ncol % 8 == 0) {
    int nblock = ncol / 8;
    size_t nwork = (size_t)(3 * nblock - 2) * 64;
    double *block_work =
        (double *)pmalloc((nwork + (size_t)ncol) * sizeof(double));
    double *saved_b = block_work + nwork;
    memcpy(saved_b, b, ncol * sizeof(double));
    solved_by_blocks = c_solve_block_tridiag8(cband, nblock, b, ipvt,
                                               block_work);
    if (!solved_by_blocks) memcpy(b, saved_b, ncol * sizeof(double));
  }
  if (!solved_by_blocks) {
#ifdef __CUDA_ARCH__
    int info;
    c_sgbfa(cband, (9 * (ds->nstr / 2) - 2), ncol, ncd, ncd, ipvt,
            &info);
    if (info != 0) {
      c_errmsg("solve0--sgbfa says matrix is singular",DS_WARNING);
    }
#else
    rcond = 0.;
    c_sgbco(cband,(9*(ds->nstr/2)-2),ncol,ncd,ncd,ipvt,&rcond,z);

    if (1.+rcond == 1.) {
      c_errmsg("solve0--sgbco says matrix near singular",DS_WARNING);
    }
#endif
  }

  /*
   * Solve linear system with coeff matrix CBAND and R.H. side(s) B
   * after CBAND has been L-U decomposed. Solution is returned in B.
   */

  if (!solved_by_blocks) {
#ifdef __CUDA_ARCH__
    c_sgbsl(cband, (9 * (ds->nstr / 2) - 2), ncol, ncd, ncd, ipvt, b, 0);
#else
    c_sgbsl(cband,(9*(ds->nstr/2)-2),ncol,ncd,ncd,ipvt,b,0);
#endif
  }

  /*
   * Zero CBAND (it may contain 'foreign' elements upon returning from
   * LINPACK) before another azimuthal component reuses it.  Flux-only
   * solves exit after this component and do not reuse the matrix.
   */
  if (!ds->flag.onlyfl) {
    memset(cband,0,(9*(ds->nstr/2)-2)*(ds->nstr*ds->nlyr)*sizeof(double));
  }

  for (lc = 1; lc <= ncut; lc++) {
    ipnt = lc*ds->nstr-nn;
    for (iq = 1; iq <= nn; iq++) {
      LL(nn-iq+1,lc) = B(ipnt-iq+1);
      LL(nn+iq,  lc) = B(ipnt+iq  );
    }
  }

  return;
}

/*============================= c_solve1() ===============================*/

/*
     Construct right-hand side vector -b- for isotropic incidence
     (only) on either top or bottom boundary and solve system
     of equations obtained from the boundary conditions and the
     continuity-of-intensity-at-layer-interface equations

     I N P U T      V A R I A B L E S:

       ds       :  Disort state variables
       cband    :  Left-hand side matrix of banded linear system
                   eq. SC(5), scaled by eq. SC(12); assumed already
                   in LU-decomposed form, ready for LINPACK solver
       ihom     :  Direction-of-illumination flag (TOP_ILLUM, top; BOT_ILLUM, bottom)
       ipvt     :
       ncol     :  Number of columns in CBAND
       ncut     :
       nn       :  Order of double-Gauss quadrature (NSTR/2)

    O U T P U T     V A R I A B L E S:

       b        :  Right-hand side vector of eq. SC(5) going into
                   sgbsl; returns as solution vector of eq.
                   SC(12), constants of integration without
                   exponential term
       ll       :  permanent storage for -b-, but re-ordered


    I N T E R N A L    V A R I A B L E S:

       ipvt     :  INTEGER vector of pivot indices
       ncd      :  Number of diagonals below or above main diagonal

   Called by- c_albtrans
   Calls- c_sgbsl
 +-------------------------------------------------------------------+
*/

DISPATCH_MACRO inline void c_solve1(disort_state *ds,
              double       *cband,
              int           ihom,
              int          *ipvt,
              int           ncol,
              int           ncut,
              int           nn,
              double       *b,
              double       *ll)
{
  int
    i,ipnt,iq,lc,ncd;

  memset(b,0,ds->nstr*ds->nlyr*sizeof(double));

  if (ihom == TOP_ILLUM) {
    /*
     * Because there are no beam or emission sources, remainder of B array is zero
     */
    for (i = 1; i <= nn; i++) {
      B(i)         = ds->bc.fisot;
      B(ncol-nn+i) = 0.;
    }
  }
  else if (ihom == BOT_ILLUM) {
    for (i = 1; i <= nn; i++) {
      B(i)         = 0.;
      B(ncol-nn+i) = ds->bc.fisot;
    }
  }
  else {
    c_errmsg("solve1---unrecognized ihom",DS_ERROR);
  }

  ncd = 3*nn-1;
  c_sgbsl(cband,(9*(ds->nstr/2)-2),ncol,ncd,ncd,ipvt,b,0);
  for (lc = 1; lc <= ncut; lc++) {
    ipnt = lc*ds->nstr-nn;
    for (iq = 1; iq <= nn; iq++) {
      LL(nn-iq+1,lc) = B(ipnt-iq+1);
      LL(nn+iq,  lc) = B(ipnt+iq  );
    }
  }

  return;
}

/*============================= end of c_solve1() ========================*/
