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

/*============================= c_ratio() ===============================*/

/*
 * Calculate ratio a/b with overflow and underflow protection
 * (thanks to Prof. Jeff Dozier for some suggestions here).
 * 
 * Modification in this C version: in the case b == 0., returns 1.+a.
 *
 * Called by: c_disort
 */

DISPATCH_MACRO inline double c_ratio(double a,
             double b)
{
  int
    initialized = FALSE;
  double
    tiny,huge,powmax,powmin;
  double
    ans,absa,absb,powa,powb;

  if(!initialized) {
    tiny   = DBL_MIN;
    huge   = DBL_MAX;
    powmax = log10(huge);
    powmin = log10(tiny);
   
    initialized = TRUE;
  }

  if (c_fcmp(b,0.) == 0) {
    ans = 1.+a;
  }
  else if (c_fcmp(a,0.) == 0) {
    ans = 0.;
  }
  else {
    absa = fabs(a);
    absb = fabs(b);
    powa = log10(absa);
    powb = log10(absb);
    if (c_fcmp(absa,tiny) < 0 && c_fcmp(absb,tiny) < 0) {
      ans = 1.;
    }
    else if (c_fcmp(powa-powb,powmax) >= 0) {
      ans = huge;
    }
    else if(c_fcmp(powa-powb,powmin) <= 0) {
      ans = tiny;
    }
    else {
      ans = absa/absb;
    }

   /*
    * NOTE: Don't use old trick of determining sign from a*b because a*b
    *       may overflow or underflow.
    */
    if ( (a > 0. && b < 0.) || (a < 0. && b > 0.) ) {
      ans *= -1;
    }
  }

  return ans;
}

/*============================= end of c_ratio() ========================*/

/*============================= c_fcmp() ================================*/

/*
 * Derived from fcmp(), version 1.2.2, 
 * Copyright (c) 1998-2000 Theodore C. Belding
 * University of Michigan Center for the Study of Complex Systems
 * <mailto:Ted.Belding@umich.edu>
 * <http://fcmp.sourceforge.net>
 *
 * The major modification we have made is to remove the "epsilon" argument
 * and set epsilon inside the fcmp() function.
 *
 * Description:
 *   It is generally not wise to compare two floating-point values for
 *   exact equality, for example using the C == operator.  The function
 *   fcmp() implements Knuth's suggestions for safer floating-point
 *   comparison operators, from:
 *   Knuth, D. E. (1998). The Art of Computer Programming.
 *   Volume 2: Seminumerical Algorithms. 3rd ed. Addison-Wesley.
 *   Section 4.2.2, p. 233. ISBN 0-201-89684-2.
 *
 * Input parameters:
 *   x1, x2: numbers to be compared
 *
 * Returns:
 *   -1 if x1 < x2
 *    0 if x1 == x2
 *    1 if x1 > x2		
 */

DISPATCH_MACRO inline int c_fcmp(double x1,
           double x2) {
  int 
    exponent;
  double
    delta,
    difference;
  const double
    epsilon = DBL_EPSILON;
  
  /* 
   * Get exponent(max(fabs(x1),fabs(x2))) and store it in exponent. 
   *
   * If neither x1 nor x2 is 0,
   * this is equivalent to max(exponent(x1),exponent(x2)).
   *
   * If either x1 or x2 is 0, its exponent returned by frexp would be 0,
   * which is much larger than the exponents of numbers close to 0 in
   * magnitude. But the exponent of 0 should be less than any number
   * whose magnitude is greater than 0.
   *
   * So we only want to set exponent to 0 if both x1 and x2 are 0. 
   * Hence, the following works for all x1 and x2. 
   */
  frexp(fabs(x1) > fabs(x2) ? x1 : x2,&exponent);

  /* 
   * Do the comparison.
   *
   * delta = epsilon*pow(2,exponent)
   *
   * Form a neighborhood around x2 of size delta in either direction.
   * If x1 is within this delta neighborhood of x2, x1 == x2.
   * Otherwise x1 > x2 or x1 < x2, depending on which side of
   * the neighborhood x1 is on.
   */
  delta      = ldexp(epsilon,exponent); 
  difference = x1-x2;

  if (difference > delta) {
    /* x1 > x2 */
    return 1;
  }
  else if (difference < -delta) {
    /* x1 < x2 */
    return -1;
  }
  else  {
    /* -delta <= difference <= delta */
    return 0;  /* x1 == x2 */
  }
}

/*============================= end of c_fcmp() =========================*/

/*============================= c_errmsg() ===============================*/

/*
 * Print out a warning or error message;  abort if type == DS_ERROR
 */

#define MAX_WARNINGS 100

DISPATCH_MACRO inline void c_errmsg(char const *messag,
              int   type)
{
  int
    warning_limit = FALSE,
    num_warnings  = 0;

  if (type == DS_ERROR) {
    fprintf(stderr,"\n ******* ERROR >>>>>>  %s\n",messag);
    exit(1);
  }

  if (warning_limit) return;

  if (++num_warnings <= MAX_WARNINGS) {
    fprintf(stderr,"\n ******* WARNING >>>>>>  %s\n",messag);
  }
  else {
    fprintf(stderr,"\n\n >>>>>>  TOO MANY WARNING MESSAGES --  ','They will no longer be printed  <<<<<<<\n\n");
    warning_limit = TRUE;
  }

  return;
}

#undef MAX_WARNINGS

/*============================= end of c_errmsg() ========================*/

/*============================= c_write_bad_var() ========================*/

/*
   Write name of erroneous variable and return TRUE; count and abort
   if too many errors.

   Input : quiet  = VERBOSE or QUIET
           varnam = name of erroneous variable to be written
 ----------------------------------------------------------------------*/

DISPATCH_MACRO inline int c_write_bad_var(int   quiet,
                    char const *varnam)
{
  const int
    maxmsg = 50;
  int
    nummsg = 0;

  nummsg++;
  if (quiet != QUIET) {
    fprintf(stderr,"\n ****  Input variable %s in error  ****\n",varnam);
    if (nummsg == maxmsg) {
      c_errmsg("Too many input errors.  Aborting...",DS_ERROR);
    }
  }

  return TRUE;
}

/*============================= end of c_write_bad_var() =================*/

/*============================= c_write_too_small_dim() ==================*/

/*
    Write name of too-small symbolic dimension and the value it should be
    increased to;  return TRUE

    Input :  quiet  = VERBOSE or QUIET
             dimnam = name of symbolic dimension which is too small
             minval = value to which that dimension should be increased
 ----------------------------------------------------------------------*/

DISPATCH_MACRO inline int c_write_too_small_dim(int   quiet,
                          char const *dimnam,
                          int   minval)
{
  if (quiet != QUIET) {
    fprintf(stderr," ****  Symbolic dimension %s should be increased to at least %d  ****\n",
            dimnam,minval);
  }

  return TRUE;
}

/*============================= end of c_write_too_small_dim =============*/

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 Call tree:

   c_sgbco
       c_sasum
       c_sdot
       c_saxpy
       c_sgbfa
           c_isamax
           c_saxpy
           c_sscal
       c_sscal
   c_sgbsl
       c_sdot
       c_saxpy
   c_sgeco
       c_sasum
       c_sdot
       c_saxpy
       c_sgefa
           c_isamax
           c_saxpy
           c_sscal
       c_sscal
   c_sgesl
       c_sdot
       c_saxpy
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

