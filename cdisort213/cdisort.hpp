/************************************************************************
 * $Id: cdisort.c 2966 2013-07-24 08:58:48Z svn-kylling $
 ************************************************************************/

/*
 *   Copyright (c) 2011 by Timothy E. Dowling
 *   
 *   This file is part of cdisort.
 *
 *   cdisort is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   cdisort is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with cdisort.  If not, see <http://www.gnu.org/licenses/>.
 */
 
/*
 * DISORT: Discrete Ordinates Radiative Transfer Program
 *
 * Version 2.1.2 (rewritten in C from Fortran DISORT.f, v 2.1)
 *
 * The C-version has the following extensions NOT present in the original fortran version:
 *  - Correction of intensity fields by Buras-Emde algorithm, included by Robert Buras.
 *  - Pseudospherical geometry for direct beam source, included by Arve Kylling.
 *  - Solution for a general source term, included by Arve Kylling.
 *
 * This file, cdisort.c, contains the source code for disort(), twostr(), and supporting subroutines.
 * The corresponding header file is cdisort.h.
 *
 * See DISORT.txt for a full description and history of the Fortran version.
 *
 *-----------------------REFERENCES (cited in code using the acronyms shown)------------------------------------
 *
 *     BDE: Buras R, Dowling T, Emde C, 201X, ...
 *    DGIS: Devaux C, Grandjean P, Ishiguro Y, Siewert CE, 1979, 
 *              On Multi-Region Problems in Radiative Transfer, Astrophys. Space Sci. 62, 225-233
 *      GS: Garcia RDM, Siewert CE, 1985, Benchmark Results in Radiative Transfer,
 *              Transport Theory and Statistical Physics 14, 437-483
 *      KS: Kylling A, Stamnes K, 1992, Efficient yet accurate solution of the linear transport
 *              equation in the presence of internal sources: The exponential-linear-in-depth
 *              approximation, J. Comp. Phys., 102, 265-276
 *       L: Lenoble J, ed, 1985:  Radiative Transfer in Absorbing
 *              and Scattering Atmospheres: Standard Computational Procedures, Deepak Publishing, Hampton, Virginia
 *      NT: Nakajima T, Tanaka M, 1988,  Algorithms for Radiative Intensity Calculations in 
 *              Moderately Thick Atmospheres Using a Truncation Approximation, J.Q.S.R.T. 40, 51-69
 *      OS: Ozisik M, Shouman S, 1980,  Source Function Expansion Method for Radiative Transfer in a Two-Layer
 *              Slab, J.Q.S.R.T. 24, 441-449
 *      SS: Stamnes K, Swanson R, 1981,  A New Look at the Discrete Ordinate Method for Radiative Transfer
 *              Calculations in Anisotropically Scattering Atmospheres, J. Atmos. Sci. 38, 387-399
 *      SD: Stamnes K, Dale H, 1981, A New Look at the Discrete Ordinate Method for Radiative Transfer
 *              Calculations in Anisotropically Scattering Atmospheres. II: Intensity Computations,
 *              J. Atmos. Sci. 38, 2696-2706
 *      S1: Stamnes K, 1982, On the Computation of Angular Distributions of Radiation in Planetary
 *              Atmospheres, J.Q.S.R.T. 28, 47-51
 *      S2: Stamnes K, 1982, Reflection and Transmission by a Vertically Inhomogeneous Planetary Atmosphere,
 *              Planet. Space Sci. 30, 727-732
 *      SC: Stamnes K, Conklin P, 1984, A New Multi-Layer Discrete Ordinate Approach to Radiative Transfer
 *              in Vertically Inhomogeneous Atmospheres, J.Q.S.R.T. 31, 273-282
 *      SW: Sweigart A, 1970, Radiative Transfer in Atmospheres Scattering According to the Rayleigh Phase Function
 *              with Absorption, The Astrophysical Journal Supplement Series 22, 1-80
 *    STWJ: Stamnes K, Tsay SC, Wiscombe W, Jayaweera K, 1988, A Numerically Stable Algorithm for
 *              Discrete-Ordinate-Method Radiative Transfer in Multiple Scattering and Emitting Layered Media,
 *              Appl. Opt. 27, 2502-2509
 *    STWL: Stamnes K, Tsay SC, Wiscombe W, Laszlo I: A General-Purpose Numerically Stable Computer
 *              Code for Discrete-Ordinate-Method Radiative Transfer in Scattering and Emitting Layered Media,
 *              DISORT Report v1.1 (2000)
 * VH1,VH2: Van de Hulst, H.C., 1980: Multiple Light Scattering, Tables, Formulas and Applications, Volumes 1 and 2,
 *              Academic Press, New York.
 *       W: Wiscombe, W., 1977:  The Delta-M Method: Rapid Yet Accurate Radiative Flux Calculations, J. Atmos. Sci.
 *              34, 1408-1422
 *-----------------------------------------------------------------------------------------------------------------
 *
 * This file contains the following functions:
 *
 *   c_disort()...................Plane-parallel discrete ordinates radiative transfer program
 *   c_bidir_reflectivity().......Supplies surface bi-directional reflectivity (Fortran name bdref).
 *   c_getmom()...................Calculate phase function Legendre expansion coefficients in various special
 *                                cases.
 *   c_asymmetric_matrix()........Solve eigenfunction problem for real asymmetric matrix known a priori
 *                                to have real eigenvalues (Fortran name asymtx).
 *   c_intensity_components().....Calculate the Fourier intensity components at the quadrature angles for azimuthal
 *                                expansion terms (mazim) in eq. SD(2), STWL(6) (Fortran name cmpint).
 *   c_fluxes()...................Calculate the radiative fluxes, mean intensity, and flux derivative with respect
 *                                to optical depth from the m=0 intensity components (the azimuthally-averaged
 *                                intensity).
 *   c_intensity_correction().....Correct intensity field by using Nakajima-Tanaka (1988) algorithm (Fortran name
 *                                intcor).
 *   c_secondary_scat()...........Calculate secondary scattered intensity of eq. STWL (A7) (Fortran name secsca).
 *   c_new_intensity_correction().Correct intensity field by using Buras-Emde (201X) algorithm (Fortran name
 *                                intcor3).
 *   prep_double_scat_integr()....Prepare integration for double scattering intensity correction (see BDE).
 *   c_new_secondary_scat().......Calculate secondary scattered intensity of eq. BDE (XX) (Fortran name secsca3).
 *   calc_phase_squared().........Calculate double scattering intensity correction (see BDE).
 *   c_disort_set()...............Perform misc. setting-up operations (Fortran name setdis).
 *   c_set_matrix()...............Calculate coefficient matrix for the set of equations obtained from the boundary
 *                                conditions and the continuity-of-intensity-at-layer-interface equations (Fortran
 *                                name setmtx).
 *   c_single_scat()..............Calculates single-scattered intensity from eqs. STWL (65b,d,e) (Fortran name
 *                                sinsca).
 *   c_solve_eigen()..............Solve eigenvalue/vector problem necessary to construct homogeneous part of
 *                                discrete ordinate solution; STWJ(8b), STWL(23f) (Fortran name soleig).
 *   c_solve0()...................Construct right-hand side vector -b- for general boundary conditions STWJ(17) and
 *                                solve system of eqns. obtained from the b.c.s and the
 *                                continuity-of-intensity-at-layer-interface eqns.
 *   c_solve1()...................Construct right-hand side vector -b- for isotropic incidence (only) on either top
 *                                or bottom boundary and solve system of eqns. obtained from the b.c.s and the
 *                                continuity-of-intensity-at-layer-interface eqns.
 *   c_twostr_solve_bc()..........Construct right-hand side vector -b- for general b.c. and solve system of eqns.
 *                                obtained from the b.c.s and the continuity-of-intensity-at-layer-interface eqns.
 *   c_surface_bidir()............Compute user's surface bidirectional properties, STWL(41) (Fortran name surfac).
 *   c_interp_eigenvec()..........Interpolate eigenvectors to user angles; eq SD(8) (Fortran name terpev).
 *   c_interp_source()............Interpolate source functions to user angles, eq. STWL(30) (Fortran name terpso).
 *   c_upbeam()...................Find the incident-beam particular solution of SS(18), STWL(24a).
 *   c_upisot()...................Find the particular solution of thermal radiation of STWL(25).
 *   c_user_intensities().........Compute intensity components at user output angles for azimuthal expansion terms
 *                                in eq. SD(2), STWL(6) (Fortran name usrint).
 *   c_xi_fun()...................Calculates Xi function of eq. STWL (72) (Fortran name xifunc).
 *   c_check_inputs().............Check the input dimensions and variables (Fortran name chekin).
 *   c_dref().....................Flux albedo for given angle of incidence, given a bidirectional reflectivity.
 *   c_legendre_poly()............Compute the normalized associated Legendre polynomial, defined in terms of the
 *                                associated Legendre polynomial (Fortran name lepoly).
 *   c_planck_func1().............Compute Planck function integrated between two wavenumbers (Fortran name plkavg).
 *   c_planck_func2().............Compute Planck function integrated between two wavenumbers, or Planck function at
 *                                a specific wavenumber (Fortran name tplkavg).
 *   c_print_avg_intensities()....Print azimuthally averaged intensities at user angles (Fortran name pravin).
 *   c_print_inputs().............Print values of input variables (Fortran name prtinp).
 *   c_print_intensities()........Print the intensity at user polar and azimuthal angles (Fortran name prtint).
 *   c_gaussian_quadrature()......Compute weights and abscissae for ordinary Gaussian quadrature on the interval
 *                                (0,1); that is, such that sum(i=1 to M) ( GWT(i) f(GMU(i)) ) is a good
 *                                approximation to integral(0 to 1) (f(x) dx) (Fortran name qgausn).
 *   c_ratio()....................Returns ratio a/b with overflow and underflow protection, or 1.+a if b == 0.
 *   c_self_test()................Sets up self test and compares results (Fortran name slftst).
 *   c_albtrans().................DISORT special case to get only albedo and transmissivity of entire medium
 *                                as a function of incident beam angle (Fortran name albtrn).
 *   c_albtrans_intensity().......Computes azimuthally-averaged intensity at top and bottom of medium (related to
 *                                albedo and transmission of medium by reciprocity principles; see Ref S2; Fortran
 *                                name altrin).
 *   c_print_albtrans()...........Print planar albedo and transmissivity of medium as a function of incident beam
 *                                angle (Fortran name praltr).
 *   c_albtrans_spherical().......Calculate spherical albedo and transmissivity for the entire medium from the m=0
 *                                intensity components; a specialized version of fluxes (Fortran name spaltr).
 *   c_errmsg()...................Print out a warning or error message; abort if error.
 *   c_write_bad_var()............Write names of erroneous variable, keep count (Fortran name wrtbad, wrtbad2).
 *   c_write_too_small_dim()......Write name of too-small symbolic dimension and the value to which it should be
 *                                increased (Fortran name wrtdim, wrtdim2).
 *   c_sgbco()....................Factor a band matrix by Gaussian elimination and estimate the condition of the
 *                                matrix.
 *   c_sgbfa()....................Factor a band matrix by elimination.
 *   c_afval()....................Solve the band system A*X = B or transpose(A)*X = B using the factors computed by
 *                                sgbco() or sgbfa().
 *   c_sgeco()....................Factor a matrix by Gaussian elimination and estimate the condition of the matrix.
 *   c_sgefa()....................Factor a matrix by Gaussian elimination.
 *   c_sgesl()....................Solve the system A*X = B or transpose(A)*X = B using the factors computed by
 *                                sgeco() or sgefa().
 *   c_sasum()....................Sum of absolute values of elements in an array.
 *   c_saxpy()....................Compute A*X + Y given scalar A and vectors X and Y.
 *   c_sdot().....................Dot product (inner product) of input vectors X and Y.
 *   c_sscal()....................Multiply given scalar and vector.
 *   c_isamax()...................Return biggest absolute value among the array elements.
 *   c_twostr()...................Solve the radiative transfer equation in the two-stream approximation.
 *                                Based on the general-purpose algorithm DISORT, but both simplified and extended.
 *   c_chapman()..................Calculate the Chapman factor.
 *   c_twostr_check_inputs()......Check input dimensions and variables for two stream code (Fortran name tchekin).
 *   c_twostr_fluxes()............Calculate radiative fluxes, mean intensity, and flux derivative with respect to
 *                                optical depth from the azimuthally-averaged intensity (Fortran name tfluxes).
 *   c_twostr_solns().............Calculates the homogenous and particular solutions to the radiative transfer
 *                                equation in the two-stream approximation, for each layer in the medium (Fortran
 *                                name hopsol).
 *   c_twostr_print_inputs()......Print values of input variables (Fortran name tprtinp).
 *   c_twostr_set()...............Perform miscellaneous setting-up operations (Fortran name settwo).
 *
 * Functions added to C version:
 *
 *   c_fcmp().....................Safe floating-point comparison function (more reliable than ==)
 *   c_disort_state_alloc().......Dynamically allocate memory for disort input arrays, incl. ones the user can ask
 *                                disort() to calculate
 *   c_disort_state_free()........Free memory allocated by disort_state_alloc()
 *   c_disort_out_alloc_()........Dynamically allocate memory for disort output arrays
 *   c_disort_out_free()..........Free memory allocated by disort_out_alloc()
 *   c_twostr_state_alloc().......Dynamically allocate memory for twostr input arrays
 *   c_twostr_state_free(0........Free memory allocated by twostr_state_alloc()
 *   c_twostr_out_alloc().........Dynamically allocate memory for twostr output arrays
 *   c_twostr_out_free()..........Free memory allocated by twostr_out_alloc()
 *   c_dbl_vector()...............Allocate zeroed memory for double-precision vector of given range
 *   c_int_vector()...............Allocate zeroed memory for integer vector of given range
 *   c_free_dbl_vector()..........Free memory allocated by dbl_vector()
 *
 *  C rewrite by Timothy E. Dowling (Univ. of Louisville)
 *  This rewrite includes conversion to double precision, dynamic memory allocation, introduction of C structures
 *  (which may yield beneficial cache-aware memory allocation), and improved readability of subroutine names.
 *  new intensity correction added by Robert Buras (LMU Munich)
 */

/* cdisort.hpp -- umbrella header for the header-only cdisort.
 *
 * cdisort.c was split into the per-module impl headers included below so
 * that every solver function is DISPATCH_MACRO inline and can compile
 * for both host and CUDA device (one GPU thread = one column).  Include
 * THIS header (not the impl headers directly) from every translation
 * unit that calls a c_* function.
 */
#pragma once

#include "cdisort.h"
#include "locate.h"
#include "pmem.h"

/* Device trajectory: cdisort's only host-only calls are fprintf (always
 * with a stream argument) and exit.  Mapping them here keeps the split
 * function bodies verbatim; the shim is scoped to these includes. */
#if defined(__CUDA_ARCH__)
#define fprintf(stream, ...) printf(__VA_ARGS__)
#define exit(code) __trap()
#endif

#include "errmsg.h"
#include "alloc.h"
#include "planck.h"
#include "quadrature.h"
#include "linpack.h"
#include "disort_set.h"
#include "solve.h"
#include "interp.h"
#include "upbeam.h"
#include "intensity.h"
#include "fluxes.h"
#include "albtrans.h"
#include "print.h"
#include "self_test.h"
#include "twostr.h"
#include "disort.h"
#include "fast_flux.h"

#if defined(__CUDA_ARCH__)
#undef fprintf
#undef exit
#endif
