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

/*============================= c_sgbco() ================================*/

/*
     Factors a real band matrix by Gaussian elimination and estimates the
     condition of the matrix.
     Revision date:  8/1/82
     Author:  Moler, C.B. (Univ. of New Mexico)

     If  RCOND  is not needed, sgbfa is slightly faster.
     To solve  A*X = B , follow sgbco by sgbsl.

     Inputs:
        abd     double(LDA,N), contains the matrix in band storage.
                The columns of the matrix are stored in the columns of abd
                and the diagonals of the matrix are stored in rows
                ml+1 through 2*ml+mu+1 of  abd.
                See the comments below for details.
        lda     int, the leading dimension of the array abd.
                lda must be >= 2*ml+mu+1.
        n       int,the order of the original matrix.
        ml      int, number of diagonals below the main diagonal.
                0 <= ml < n.
        mu      int, number of diagonals above the main diagonal.
                0 <= mu < n.
                more efficient if  ml <= mu.

     Outputs:
        abd     an upper triangular matrix in band storage and
                the multipliers which were used to obtain it.
                The factorization can be written  A = L*U  where
                L  is a product of permutation and unit lower
                triangular matrices and  U  is upper triangular.
        ipvt    int[n], an integer vector of pivot indices.
        rcond   double, an estimate of the reciprocal condition of A.
                For the system  A*X = B, relative perturbations
                in A and B of size epsilon may cause relative
                perturbations in  X  of size  epsilon/rcond.
                If rcond  is so small that the logical expression
                   1.+RCOND == 1.
                is true, then  A  may be singular to working
                precision.  In particular, rcond is zero if exact
                singularity is detected or the estimate underflows.
        z       double[n], a work vector whose contents are usually
                unimportant. If A is close to a singular matrix, then
                z is an approximate null vector in the sense that
                norm(a*z) = rcond*norm(a)*norm(z).

     Band storage:
           If A is a band matrix, the following program segment
           will set up the input (with unit-offset arrays):
                   ml = (band width below the diagonal)
                   mu = (band width above the diagonal)
                   m = ml+mu+1
                   for (j = 1; j <= n; j++) {
                     i1 = IMAX(1,j-mu);
                     i2 = IMIN(n,j+ml);
                     for (i = i1; i <= i2; i++) {
                       k = i-j+m;
                       ABD(K,J) = A(I,J);
                     }
                   }
           This uses rows ml+1 through 2*ml+mu+1 of abd.
           In addition, the first ml rows in abd are used for
           elements generated during the triangularization.
           The total number of rows needed in abd is 2*ml+mu+1.
           The ml+mu by ml+mu upper left triangle and the
           ml by ml lower right triangle are not referenced.

     Example:  if the original matrix is

           11 12 13  0  0  0
           21 22 23 24  0  0
            0 32 33 34 35  0
            0  0 43 44 45 46
            0  0  0 54 55 56
            0  0  0  0 65 66

      then  n = 6, ml = 1, mu = 2, lda >= 5  and abd should contain
            *  *  *  +  +  +  , * = not used
            *  * 13 24 35 46  , + = used for pivoting
            * 12 23 34 45 56
           11 22 33 44 55 66
           21 32 43 54 65  *

 --------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_sgbco(double *abd,
             int     lda,
             int     n,
             int     ml,
             int     mu,
             int    *ipvt,
             double *rcond,
             double *z)
{
  int
    info;
  int
    is,j,ju,k,kb,kp1,l,la,lm,lz,m,mm;
  double
    anorm,ek,s,sm,t,wk,wkm,ynorm;

  /*
   * compute 1-norm of A
   */
  anorm = 0.;
  l  = ml+1;
  is = l+mu;
  for (j = 1; j <= n; j++) {
    anorm = MAX(anorm,c_sasum(l,&ABD(is,j)));
    if (is > ml+1) {
      is--;
    }
    if (j <= mu) {
      l++;
    }
    if (j >= n-ml) {
      l--;
    }
  }
  /*
   * factor
   */
  c_sgbfa(abd,lda,n,ml,mu,ipvt,&info);
  /*
   * rcond = 1/(norm(A)*(estimate of norm(inverse(A)))) .
   * estimate = norm(Z)/norm(Y) where  A*Z = Y  and  trans(A)*Y = E.
   * trans(A) is the transpose of A.  The components of E are
   * chosen to cause maximum local growth in the elements of W where
   * trans(U)*W = E. The vectors are frequently rescaled to avoid overflow.
   * solve trans(U)*W = E
   */
  ek = 1.;

  memset(z,0,n*sizeof(double));

  m  = ml+mu+1;
  ju = 0;
  for (k = 1; k <= n; k++) {
    if (Z(k) != 0.) {
      ek = F77_SIGN(ek,-Z(k));
    }
    if (fabs(ek-Z(k)) > fabs(ABD(m,k))) {
      s = fabs(ABD(m,k))/fabs(ek-Z(k));
      c_sscal(n,s,z);
      ek *= s;
    }
    wk  =  ek-Z(k);
    wkm = -ek-Z(k);
    s   = fabs(wk);
    sm  = fabs(wkm);
    if (ABD(m,k) != 0.) {
      wk  /= ABD(m,k);
      wkm /= ABD(m,k);
    }
    else {
      wk  = 1.;
      wkm = 1.;
    }
    kp1 = k+1;
    ju  = IMIN(IMAX(ju,mu+IPVT(k)),n);
    mm  = m;
    if (kp1 <= ju) {
      for (j = kp1; j <= ju; j++) {
        mm--;
        sm   += fabs(Z(j)+wkm*ABD(mm,j));
        Z(j) += wk*ABD(mm,j);
        s    += fabs(Z(j));
      }
      if (s < sm) {
        t  = wkm-wk;
        wk = wkm;
        mm = m;
        for (j = kp1; j <= ju; j++) {
          mm--;
          Z(j) += t*ABD(mm,j);
        }
      }
    }
    Z(k) = wk;
  }

  s = 1./c_sasum(n,z);
  c_sscal(n,s,z);

  /*
   * solve trans(L)*Y = W
   */
  for (kb = 1; kb <= n; kb++) {
    k  = n+1-kb;
    lm = IMIN(ml,n-k);
    if (k < n) {
      Z(k) += c_sdot(lm,&ABD(m+1,k),&Z(k+1));
    }
    if (fabs(Z(k)) > 1.) {
      s = 1./fabs(Z(k));
      c_sscal(n,s,z);
    }

    l    = IPVT(k);
    t    = Z(l);
    Z(l) = Z(k);
    Z(k) = t;
  }

  s = 1./c_sasum(n,z);
  c_sscal(n,s,z);

  ynorm = 1.;
  /*
   * solve L*V = Y
   */
  for (k = 1; k <= n; k++) {
    l    = IPVT(k);
    t    = Z(l);
    Z(l) = Z(k);
    Z(k) = t;
    lm   = IMIN(ml,n-k);
    if (k < n) {
      c_saxpy(lm,t,&ABD(m+1,k),&Z(k+1));
    }
    if (fabs(Z(k)) > 1.) {
      s = 1./fabs(Z(k));
      c_sscal(n,s,z);
      ynorm *= s;
    }
  }

  s = 1./c_sasum(n,z);
  c_sscal(n,s,z);

  ynorm *= s;
  /*
   * solve  U*Z = W
   */
  for (kb = 1; kb <= n; kb++) {
    k = n+1-kb;
    if (fabs(Z(k)) > fabs(ABD(m,k))) {
      s = fabs(ABD(m,k))/fabs(Z(k));
      c_sscal(n,s,z);
      ynorm *= s;
    }
    if (ABD(m,k) != 0.) {
      Z(k) /= ABD(m,k);
    }
    else {
      Z(k) = 1.;
    }
    lm = IMIN(k,m)-1;
    la = m-lm;
    lz = k-lm;
    t  = -z[k-1];
    c_saxpy(lm,t,&ABD(la,k),&Z(lz));
  }

  /*
   * make znorm = 1.
   */
  s = 1./c_sasum(n,z);
  c_sscal(n,s,z);

  ynorm *= s;
  if(anorm != 0.) {
    *rcond = ynorm/anorm;
  }
  else {
    *rcond = 0.;
  }

  return;
}

/*============================= end of c_sgbco() =========================*/

/*============================= c_sgbfa() ================================*/

/*
    Factors a real band matrix by elimination.
    Revision date:  8/1/82
    Author:  Moler, C. B. (U. of New Mexico)
    c_sgbfa is usually called by c_sgbco, but it can be called
    directly with a saving in time if rcond is not needed.

    Inputs:  same as c_sgbco
    Outputs:
        abd,ipvt    same as c_sgbco
        info    int,
                = 0  normal value.
                = k  if  u(k,k) == 0.  This is not an error
                     condition for this subroutine, but it does
                     indicate that sgbsl will divide by zero if
                     called.  Use  rcond  in c_sgbco for a reliable
                     indication of singularity.
    (see c_sgbco for description of band storage mode)

    NOTE: using memset() to zero columns in abd
 ----------------------------------------------------------------*/

DISPATCH_MACRO inline void c_sgbfa(double *abd,
             int     lda,
             int     n,
             int     ml,
             int     mu,
             int    *ipvt,
             int    *info)
{
  int
    i0,j,j0,j1,ju,jz,k,kp1,l,lm,m,mm,nm1;
  double
    t;

  m     = ml+mu+1;
  *info = 0;
  /*
   * zero initial fill-in columns
   */
  j0 = mu+2;
  j1 = IMIN(n,m)-1;
  for (jz = j0; jz <= j1; jz++) {
    i0 = m+1-jz;
    memset(&ABD(i0,jz),0,(ml-i0+1)*sizeof(double));
  }
  jz = j1;
  ju = 0;

  /*
   * Gaussian elimination with partial pivoting
   */
  nm1 = n-1;
  for (k = 1; k <= nm1; k++) {
    kp1 = k+1;
   /*
    * zero next fill-in column
    */
    jz++;
    if (jz <= n) {
      memset(&ABD(1,jz),0,ml*sizeof(double));
    }
    /*
     * find L = pivot index
     */
    lm      = IMIN(ml,n-k);
    l       = c_isamax(lm+1,&ABD(m,k))+m-1;
    IPVT(k) = l+k-m;
    if (ABD(l,k) == 0.) {
     /*
      * zero pivot implies this column already triangularized
      */
      *info = k;
    }
    else {
      /*
       * interchange if necessary
       */
      if (l != m) {
        t        = ABD(l,k);
        ABD(l,k) = ABD(m,k);
        ABD(m,k) = t;
      }
      /*
       * compute multipliers
       */
      t = -1./ABD(m,k);
      c_sscal(lm,t,&ABD(m+1,k));
      /*
       * row elimination with column indexing
       */
      ju = IMIN(IMAX(ju,mu+IPVT(k)),n);
      mm = m;
      for (j = kp1; j <= ju; j++) {
        l--;
        mm--;
        t = ABD(l,j);
        if (l != mm) {
          ABD(l,j)  = ABD(mm,j);
          ABD(mm,j) = t;
        }
        c_saxpy(lm,t,&ABD(m+1,k),&ABD(mm+1,j));
      }
    }
  }
  IPVT(n) = n;
  if (ABD(m,n) == 0.) {
    *info = n;
  }

  return;
}

/*============================= end of c_sgbfa() =========================*/

/*============================= c_sgbsl() ================================*/

/*
    Solves the real band system
       A * X = B  or  transpose(A) * X = B
    using the factors computed by sgbco or sgbfa.
    Revision date:  8/1/82
    Author:  Moler, C. B. (Univ. of New Mexico)

    Inputs:
        abd     double(lda, n), the output from sgbco or sgbfa.
        lda     int, the leading dimension of the array abd.
        n       int, the order of the original matrix.
        ml      int, number of diagonals below the main diagonal.
        mu      int, number of diagonals above the main diagonal.
        ipvt    int(n), the pivot vector from sgbco or sgbfa.
        b       double(n), the right hand side vector.
        job     int,
                = 0         to solve  A*X = B ,
                = nonzero   to solve  transpose(A)*X = B

     Outputs:
        b       the solution vector  X

     Error condition:
        A division by zero will occur if the input factor contains a
        zero on the diagonal.  Technically, this indicates singularity,
        but it is often caused by improper arguments or improper
        setting of lda.  It will not occur if the subroutines are
        called correctly and if c_sgbco has set rcond > 0.0
        or sgbfa has set info = 0 .
     To compute  inverse(a)*c  where c is a matrix
     with p columns
      c_sgbco(abd,lda,n,ml,mu,ipvt,&rcond,z)
      if (rcond is too small) ...
        for (j = 1; j <= p; j++) {
          c_sgbsl(abd,lda,n,ml,mu,ipvt,c(1,j),0)
        }
 --------------------------------------------------------*/

DISPATCH_MACRO inline void c_sgbsl(double *abd,
             int     lda,
             int     n,
             int     ml,
             int     mu,
             int    *ipvt,
             double *b,
             int     job)
{
  int
    k,kb,l,la,lb,lm,m,nm1;
  double
    t;

  m   = mu+ml+1;
  nm1 = n-1;
  if (job == 0) {
   /*
    * solve  A*X = B;  first solve L*Y = B
    */
    if (ml != 0) {
      for (k = 1; k <= nm1; k++) {
        lm = IMIN(ml,n-k);
        l  = IPVT(k);
        t  = B(l);
        if (l != k) {
          B(l) = B(k);
          B(k) = t;
        }
        c_saxpy(lm,t,&ABD(m+1,k),&B(k+1));
      }
    }
    /*
     * now solve  U*X = Y
     */
    for (kb = 1; kb <= n; kb++) {
      k     = n+1-kb;
      B(k) /= ABD(m,k);
      lm    = IMIN(k,m)-1;
      la    = m-lm;
      lb    = k-lm;
      t     = -B(k);
      c_saxpy(lm,t,&ABD(la,k),&B(lb));
    }
  }
  else {
    /*
     * solve  trans(A)*X = B; first solve trans(U)*Y = B
     */
    for (k = 1; k <= n; k++) {
      lm   = IMIN(k,m)-1;
      la   = m-lm;
      lb   = k-lm;
      t    = c_sdot(lm,&ABD(la,k),&B(lb));
      B(k) = (B(k)-t)/ABD(m,k);
    }
    /*
     * now solve trans(L)*X = Y
     */
    if (ml != 0) {
      for (kb = 1; kb <= nm1; kb++) {
        k     = n-kb;
        lm    = IMIN(ml,n-k);
        B(k) += c_sdot(lm,&ABD(m+1,k),&B(k+1));
        l     = IPVT(k);
        if (l != k) {
          t    = B(l);
          B(l) = B(k);
          B(k) = t;
        }
      }
    }
  }

  return;
}

/*============================= end of c_sgbsl() =========================*/

/*============================= c_sgeco() ================================*/

/*
   Factors a real matrix by Gaussian elimination
   and estimates the condition of the matrix.
   Revision date:  8/1/82
   Author:  Moler, C. B. (Univ. of New Mexico)
   If rcond is not needed, sgefa is slightly faster.
   To solve  A*X = B, follow sgeco by sgesl.

     Inputs:
        a       double(lda, n), the matrix to be factored.
        lda     int, the leading dimension of the array a.
        n       int, the order of the matrix a.

     Outputs:
        a       an upper triangular matrix and the multipliers
                which were used to obtain it.
                The factorization can be written  A = L*U , where
                L  is a product of permutation and unit lower
                triangular matrices and U is upper triangular.
        ipvt    int(n), an integer vector of pivot indices.
        rcond   double, an estimate of the reciprocal condition of a.
                For the system A*X = B, relative perturbations
                in A and B of size epsilon may cause relative
                perturbations in X of size epsilon/rcond.
                If rcond is so small that the logical expression
                  1.+rcond == 1.
                is true, then A may be singular to working precision.
                In particular, rcond is zero if exact singularity
                is detected or the estimate underflows.
        z       double(n), a work vector whose contents are usually
                unimportant. If A is close to a singular matrix, then z 
                is an approximate null vector in the sense that
                norm(A*Z) = rcond*norm(A)*norm(Z) .
 ------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_sgeco(double *a,
             int     lda,
             int     n,
             int    *ipvt,
             double *rcond,
             double *z)
{
  int
    info;
  int
    j,k,kb,kp1,l;
  double
    anorm,ek,s,sm,t,wk,wkm,ynorm;

  /*
   * compute 1-norm of A
   */
  anorm = 0.;
  for (j = 1; j <= n; j++) {
    anorm = MAX(anorm,c_sasum(n,&A(1,j)));
  }

  /*
   * factor
   */
  c_sgefa(a,lda,n,ipvt,&info);

  /*
   * rcond = 1/(norm(A)*(estimate of norm(inverse(A)))).
   * estimate = norm(Z)/norm(Y) where A*Z = Y and trans(A)*Y = E.
   * trans(A) is the transpose of A. The components of E are
   * chosen to cause maximum local growth in the elements of W where
   * trans(U)*W = E.  The vectors are frequently rescaled to avoid overflow.
   * solve trans(U)*W = E
   */
  ek = 1.;
  memset(z,0,n*sizeof(double));

  for (k = 1; k <= n; k++) {
    if (Z(k) != 0.) {
      ek = F77_SIGN(ek,-Z(k));
    }
    if (fabs(ek-Z(k)) > fabs(A(k,k))) {
      s = fabs(A(k,k))/fabs(ek-Z(k));
      c_sscal(n,s,z);
      ek *= s;
    }
    wk  =  ek-Z(k);
    wkm = -ek-Z(k);
    s   = fabs(wk);
    sm  = fabs(wkm);
    if (A(k,k) != 0.) {
      wk  /= A(k,k);
      wkm /= A(k,k);
    }
    else {
      wk  = 1.;
      wkm = 1.;
    }
    kp1 = k+1;
    if (kp1 <= n) {
      for (j = kp1; j <= n; j++) {
        sm   += fabs(Z(j)+wkm*A(k,j));
        Z(j) += wk*A(k,j);
        s    += fabs(Z(j));
      }
      if (s < sm) {
        t  = wkm-wk;
        wk = wkm;
        for (j = kp1; j <= n; j++) {
          Z(j) += t*A(k,j);
        }
      }
    }
    Z(k) = wk;
  }

  s = 1./c_sasum(n,z);
  c_sscal(n,s,z);
  /*
   * solve trans(L)*Y = W
   */
  for (kb = 1; kb <= n; kb++) {
    k = n+1-kb;
    if (k < n) {
      Z(k) += c_sdot(n-k,&A(k+1,k),&Z(k+1));
    }
    if (fabs(Z(k)) > 1.) {
      s = 1./fabs(Z(k));
      c_sscal(n,s,z);
    }
    l    = IPVT(k);
    t    = Z(l);
    Z(l) = Z(k);
    Z(k) = t;
  }
  s = 1./c_sasum(n,z);
  c_sscal(n,s,z);
  /*
   * solve L*V = Y
   */
  ynorm = 1.;
  for (k = 1; k <= n; k++) {
    l    = IPVT(k);
    t    = Z(l);
    Z(l) = Z(k);
    Z(k) = t;
    if (k < n) {
      c_saxpy(n-k,t,&A(k+1,k),&Z(k+1));
    }
    if (fabs(Z(k)) > 1.) {
      s = 1./fabs(Z(k));
      c_sscal(n,s,z);
      ynorm *= s;
    }
  }
  s = 1./c_sasum(n,z);
  c_sscal(n,s,z);
  /*
   * solve U*Z = V
   */
  ynorm *= s;
  for (kb = 1; kb <= n; kb++) {
    k = n+1-kb;
    if (fabs(Z(k)) > fabs(A(k,k))) {
      s = fabs(A(k,k))/fabs(Z(k));
      c_sscal(n,s,z);
      ynorm *= s;
    }
    if (A(k,k) != 0.) {
      Z(k) /= A(k,k);
    }
    else {
      Z(k) = 1.;
    }
    t = -Z(k);
    c_saxpy(k-1,t,&A(1,k),&Z(1));
  }
  /*
   * make znorm = 1.0
   */
  s = 1./c_sasum(n,z);
  c_sscal(n,s,z);
  ynorm *= s;
  if (anorm != 0.) {
    *rcond = ynorm/anorm;
  }
  else {
    *rcond = 0.;
  }

  return;
}

/*============================= end of c_sgeco() =========================*/

/*============================= c_sgefa() ================================*/

/*
   Factors a real matrix by Gaussian elimination.
   Revision date:  8/1/82
   Author:  Moler, C. B. (Univ. of New Mexico)
   c_sgefa is usually called by c_sgeco, but it can be called directly with a
   saving in time if rcond is not needed.
   (time for c_sgeco) = (1+9/n)*(time for c_sgefa).

   Inputs:  same as c_sgeco

   Outputs:
        a,ipvt  same as c_sgeco
        info    int,
                = 0  normal value.
                = k  if  u(k,k) = 0.  This is not an error condition for
                     this subroutine, but it does indicate that c_sgesl or
                     c_sgedi will divide by zero if called.  Use rcond in
                     c_sgeco for a reliable indication of singularity.
 ---------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_sgefa(double *a,
             int     lda,
             int     n,
             int    *ipvt,
             int    *info)
{
  int
    j,k,kp1,l,nm1;
  double
    t;

  /*
   * Gaussian elimination with partial pivoting
   */
  *info = 0;
  nm1   = n-1;
  for (k = 1; k <= nm1; k++) {
    kp1 = k+1;
    /*
     * find L = pivot index
     */
    l       = c_isamax(n-k+1,&A(k,k))+k-1;
    IPVT(k) = l;
    if (A(l,k) == 0.) {
      /*
       * zero pivot implies this column already triangularized
       */
      *info = k;
    }
    else {
      /*
       * interchange if necessary
       */
      if (l != k) {
        t      = A(l,k);
        A(l,k) = A(k,k);
        A(k,k) = t;
      }
      /*
       * compute multipliers
       */
      t = -1./A(k,k);
      c_sscal(n-k,t,&A(k+1,k));
      /*
       * row elimination with column indexing
       */
      for (j = kp1; j <= n; j++) {
        t = A(l,j);
        if (l != k) {
          A(l,j) = A(k,j);
          A(k,j) = t;
        }
        c_saxpy(n-k,t,&A(k+1,k),&A(k+1,j));
      }
    }
  }
  IPVT(n) = n;
  if (A(n,n) == 0.) {
    *info = n;
  }

  return;
}

/*============================= end of c_sgefa() =========================*/

/*============================= c_sgesl() ================================*/

/*
  Solves the real system
     A*X = B  or  transpose(A)*X = B
  using the factors computed by sgeco or sgefa.
  Revision date:  8/1/82
  Author:  Moler, C. B. (Univ. of New Mexico)

     Inputs:
        a       double(lda, n), the output from sgeco or sgefa.
        lda     int, the leading dimension of the array  A
        n       int, the order of the matrix  A
        ipvt    int(n), the pivot vector from sgeco or sgefa.
        b       double(n), the right hand side vector.
        job     int, 
                = 0         to solve  A*X = B ,
                = nonzero   to solve  transpose(A)*X = B

     Outputs:
        b       the solution vector x

     Error condition:
        A division by zero will occur if the input factor contains a
        zero on the diagonal. Technically, this indicates singularity,
        but it is often caused by improper arguments or improper setting
        of lda. It will not occur if the subroutines are called correctly
        and if sgeco has set rcond > 0. or sgefa has set info = 0 .
     To compute  inverse(a)*c where c is a matrix with p columns
           c_sgeco(a,lda,n,ipvt,rcond,z);
           if (rcond is too small) ...
           for (j = 1; j <= p; j++) {
             c_sgesl(a,lda,n,ipvt,c(1,j),0);
           }
 ---------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_sgesl(double *a,
             int     lda,
             int     n,
             int    *ipvt,
             double *b,
             int     job)
{
  int
    k,kb,l,nm1;
  double
    t;

  nm1 = n-1;
  if (job == 0) {
    /*
     * solve  A*X = B; first solve L*Y = B
     */
    for (k = 1; k <= nm1; k++) {
      l = IPVT(k);
      t = B(l);
      if (l != k) {
        B(l) = B(k);
        B(k) = t;
      }
      c_saxpy(n-k,t,&A(k+1,k),&B(k+1));
    }
    /*
     * now solve  U*X = Y
     */
    for (kb = 1; kb <= n; kb++) {
      k     = n+1-kb;
      B(k) /= A(k,k);
      t     = -B(k);
      c_saxpy(k-1,t,&A(1,k),&B(1));
    }
  }
  else {
    /*
     * solve trans(A)*X = B; first solve trans(U)*Y = B
     */
    for (k = 1; k <= n; k++) {
      t    = c_sdot(k-1,&A(1,k),&B(1));
      B(k) = (B(k)-t)/A(k,k);
    }
    /*
     * now solve  trans(l)*x = y
     */
    for (kb = 1; kb <= nm1; kb++) {
      k = n-kb;
      B(k) += c_sdot(n-k,&A(k+1,k),&B(k+1));
      l     = IPVT(k);
      if (l != k) {
        t    = B(l);
        B(l) = B(k);
        B(k) = t;
      }
    }
  }

  return;
}

/*============================= end of c_sgesl() =========================*/

/*============================= c_sasum() ================================*/

/*
  Input--   n     Number of elements in vector to be summed
            sx    array, length n, containing vector

  OUTPUT--  ans   Sum from i = 1 to n of fabs(SX(i))

  NOTE: Fortran input incx removed because it is not used by
        disort or twostr
 ----------------------------------------------------------*/

DISPATCH_MACRO inline double c_sasum(int     n,
             double *sx)
{
  int
    i,m;
  double
    ans;

  ans = 0.;
  if (n <= 0) {
    return ans;
  }

  m = n%4;
  if (m != 0) {
    /*
     * clean-up loop so remaining vector length is a multiple of 4.
     */
    for (i = 1; i <= m; i++) {
      ans += fabs(SX(i));
    }
  }
  /*
   * unroll loop for speed
   */
  for (i = m+1; i <= n; i+=4) {
    ans += fabs(SX(i  ))
          +fabs(SX(i+1))
          +fabs(SX(i+2))
          +fabs(SX(i+3));
  }

  return ans;
}

/*============================= end of c_sasum() =========================*/

/*============================= c_saxpy() ================================*/

/*
  y = a*x + y  (x, y = vectors, a = scalar)

  INPUT--
        n   Number of elements in input vectors x and y
       sa   Scalar multiplier a
       sx   Array containing vector x
       sy   Array containing vector Y

 OUTPUT--
       sy   For i = 1 to n, overwrite  SY(i) with sa*SX(i)+SY(i)

  NOTE: Fortran inputs incx, incy removed because they are not used
        by disort or twostr
 ------------------------------------------------------------*/

DISPATCH_MACRO inline void c_saxpy(int     n,
             double  sa,
             double *sx,
             double *sy)
{
  int
    i,m;

  if (n <= 0 || sa == 0.) {
    return;
  }

  m = n%4;
  if (m != 0) {
    /*
     * clean-up loop so remaining vector length is a multiple of 4.
     */
    for (i = 1; i <= m; i++) {
      SY(i) += sa*SX(i);
    }
  }
  /*
   * unroll loop for speed
   */
  for (i = m+1; i <= n; i+=4) {
    SY(i  ) += sa*SX(i  );
    SY(i+1) += sa*SX(i+1);
    SY(i+2) += sa*SX(i+2);
    SY(i+3) += sa*SX(i+3);
  }

  return;
}

/*============================= c_saxpy() ================================*/

/*============================= c_sdot() =================================*/

/*
  Dot product of vectors x and y

  INPUT--
        n  Number of elements in input vectors x and y
       sx  Array containing vector x
       sy  Array containing vector y

 OUTPUT--
      ans  Sum for i = 1 to n of  SX(i)*SY(i),

  NOTE: Fortran input arguments incx, incy removed because they
        are not used in disort or twostr
 ------------------------------------------------------------------*/

DISPATCH_MACRO inline double c_sdot(int     n,
              double *sx,
              double *sy)
{
  int
    i,m;
  double
    ans;

  ans = 0.;
  if (n <= 0) {
    return ans;
  }

  m = n%4;
  if (m != 0) {
    /*
     * clean-up loop so remaining vector length is a multiple of 4.
     */
    for (i = 1; i <= m; i++) {
      ans += SX(i)*SY(i);
    }
  }
  /*
   * unroll loop for speed
   */
  for (i = m+1; i <= n; i+=4) {
    ans += SX(i  )*SY(i  )
          +SX(i+1)*SY(i+1)
          +SX(i+2)*SY(i+2)
          +SX(i+3)*SY(i+3);
  }

  return ans;
}

/*============================= end of c_sdot() ==========================*/

/*============================= c_sscal() ================================*/

/*
  Multiply vector sx by scalar sa

  INPUT--  n  Number of elements in vector
          sa  Scale factor
          sx  Array, length n, containing vector

 OUTPUT-- sx  Replace SX(i) with sa*SX(i) for i = i to n

 NOTE: Fortran input argument incx removed since it is not used
       in disort or twostr

 ---------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_sscal(int    n,
             double  sa,
             double *sx)
{
  int
    i,m;

  if (n <= 0) {
    return;
  }
  m = n%4;
  if (m != 0) {
    /*
     * clean-up loop so remaining vector length is a multiple of 4.
     */
    for (i = 1; i <= m; i++) {
      SX(i) *= sa;
    }
  }
  /*
   * unroll loop for speed
   */
  for (i = m+1; i <= n; i+=4) {
    SX(i  ) *= sa;
    SX(i+1) *= sa;
    SX(i+2) *= sa;
    SX(i+3) *= sa;
  }

  return;
}

/*============================= end of c_sscal() =========================*/

/*============================= c_isamax() ===============================*/

/*
 INPUT--  n        Number of elements in vector of interest
          sx       Array, length n, containing vector

 OUTPUT-- ans      First i, i = 1 to n, to maximize fabs(SX(i))

 NOTE: Fortran input incx removed because it is not used by
       disort or twostr
 ---------------------------------------------------------------------*/

DISPATCH_MACRO inline int c_isamax(int     n,
             double *sx)
{
  int
    ans=0,i;
  double
   smax,xmag;

  if (n <= 0) {
    ans = 0;
  }
  else if (n == 1) {
    ans = 1;
  }
  else {
    smax = 0.;
    for (i = 1; i <= n; i++) {
      xmag = fabs(SX(i));
      if (smax < xmag) {
        smax = xmag;
        ans  = i;
      }
    }
  } 

  return ans;
}

/*============================= end of c_isamax() ========================*/

