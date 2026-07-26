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

/*============================= c_fluxes() ==============================*/

/*
    Calculates the radiative fluxes, mean intensity, and flux
    derivative with respect to optical depth from the m=0 intensity
    components (the azimuthally-averaged intensity)

    I N P U T    V A R I A B L E S:

       ds       :  Disort state variables
       cmu      :  Abscissae for Gauss quadrature over angle cosine
       cwt      :  Weights for Gauss quadrature over angle cosine
       gc       :  Eigenvectors at polar quadrature angles, SC(1)
       kk       :  Eigenvalues of coeff. matrix in eq. SS(7), STWL(23b)
       layru    :  Layer number of user level UTAU
       ll       :  Constants of integration in eq. SC(1), obtained by solving scaled version of eq. SC(5);
                   exponential term of eq. SC(12) not included
       lyrcut   :  Logical flag for truncation of comput. layer
       ncut     :  Number of computational layer where absorption optical depth exceeds ABSCUT
       nn       :  Order of double-Gauss quadrature (NSTR/2)
       prntu0   :  TRUE, print azimuthally-averaged intensity at quadrature angles
       taucpr   :  Cumulative optical depth (delta-M-scaled)
       utaupr   :  Optical depths of user output levels in delta-M coordinates;  equal to UTAU if no delta-M
       xr       :  Expansion of thermal source function in eq. SS(14,16), STWL(24c); xr[].zero, xr[].one (see cdisort.h)
       zz       :  Beam source vectors in eq. SS(19), STWL(24b)
       zzg      :  Beam source vectors in eq. KS(10)for a general source constant over a layer
       plk      :  Thermal source vectors z0,z1 by solving eq. SS(16), Y0,Y1 in STWL(26b,a);
                   plk[].zero, plk[].one (see cdisort.h)

    O U T P U T    V A R I A B L E S:

       out      : Disort output variables
       u0c      :  Azimuthally averaged intensities (at polar quadrature angles)

    I N T E R N A L    V A R I A B L E S:

       dirint   :  Direct intensity attenuated
       fdntot   :  Total downward flux (direct + diffuse)
       fl       :  fl[].zero: 'fldir' = direct-beam flux (delta-M scaled), fl[].one 'fldn' = diffuse down-flux (delta-M scaled)
       fnet     :  Net flux (total_down-diffuse_up)
       fact     :  EXP(- UTAUPR/UMU0)
       plsorc   :  Planck source function (thermal)
       zint     :  Intensity of m = 0 case, in eq. SC(1)

   Called by- c_disort
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_fluxes(disort_state  *ds,
              disort_output *out,
              double        *ch,
              double        *cmu,
              double        *cwt,
              double        *gc,
              double        *kk,
              int           *layru,
              double        *ll,
              int            lyrcut,
              int            ncut,
              int            nn,
              int            prntu0,
              double        *taucpr,
              double        *utaupr,
              disort_pair   *xr,
              disort_pair   *zbeamsp,
              double        *zbeama,
              double        *zz,
              double        *zzg,
              disort_pair   *plk,
              disort_pair   *fl,
              double        *u0c)
{
  int
    iq,jq,lu,lyu;
  double
    ang1,ang2,dirint,
    fact=0,fdntot,fnet,plsorc,zint;

  if (ds->flag.prnt[1]) {
    fprintf(stdout,"\n\n                     <----------------------- FLUXES ----------------------->\n"
                   "   Optical  Compu    Downward    Downward    Downward      Upward                    Mean      Planck   d(Net Flux)\n"
                   "     Depth  Layer      Direct     Diffuse       Total     Diffuse         Net   Intensity      Source   / d(Op Dep)\n");
  }

  /*
   * Zero DISORT output arrays
   */
  memset(u0c,0,ds->ntau*ds->nstr*sizeof(double));
  memset(fl,0,ds->ntau*sizeof(disort_pair));

  /*
   * Loop over user levels
   */
  for (lu = 1; lu <= ds->ntau; lu++) {
    lyu = LAYRU(lu);

    if (lyrcut && lyu > ncut) {
      /*
       * No radiation reaches this level
       */
      fdntot = 0.;
      fnet   = 0.;
      plsorc = 0.;
      if (ds->flag.prnt[1]) {
        fprintf(stdout,"%10.4f%7d%12.3e%12.3e%12.3e%12.3e%12.3e%12.3e%12.3e%14.3e\n",
                        UTAU(lu),lyu,RFLDIR(lu),RFLDN(lu),fdntot,FLUP(lu),fnet,UAVG(lu),plsorc,DFDT(lu));
      }
      continue;
    }

    if (ds->bc.fbeam > 0.) {
      if ( ds->flag.spher == TRUE ) {
	fact         = exp( - UTAUPR(lu) / CH(lyu) );
	RFLDIR( lu ) = fabs(ds->bc.umu0)*ds->bc.fbeam*
	  exp( - UTAU( lu ) / CH(lyu) );
      }
      else {
	fact       = exp(-UTAUPR(lu)/ds->bc.umu0);
	RFLDIR(lu) = ds->bc.umu0*ds->bc.fbeam*exp(-UTAU(lu)/ds->bc.umu0);
      }
      dirint     = ds->bc.fbeam*fact;
      FLDIR(lu)  = ds->bc.umu0*ds->bc.fbeam*fact;
    }
    else {
      dirint     = 0.;
      FLDIR(lu)  = 0.;
      RFLDIR(lu) = 0.;
    }

    for (iq = 1; iq <= nn; iq++) {
      zint = 0.;
      for (jq = 1; jq <= nn; jq++) {
        zint += GC(iq,jq,lyu)*LL(jq,lyu)*exp(-KK(jq,lyu)*(UTAUPR(lu)-TAUCPR(lyu  )));
      }
      for (jq = nn+1; jq <= ds->nstr; jq++) {
        zint += GC(iq,jq,lyu)*LL(jq,lyu)*exp(-KK(jq,lyu)*(UTAUPR(lu)-TAUCPR(lyu-1)));
      }

      U0C(iq,lu) = zint;
      if (ds->bc.fbeam > 0. ) {
	if ( ds->flag.spher == TRUE ) {
	  U0C(iq,lu) += exp(-ZBEAMA(lyu)*UTAUPR(lu))*
	    ( ZBEAM0(iq,lyu)+ZBEAM1(iq,lyu)*UTAUPR(lu) );
	}
	else {
	  U0C(iq,lu) += ZZ(iq,lyu)*fact;
	}
      }
      if ( ds->flag.general_source == TRUE ) {
	U0C(iq,lu) += ZZG(iq,lyu);
      }
      U0C(iq,lu) += ZPLK0(iq,lyu)+ZPLK1(iq,lyu)*UTAUPR(lu);
      UAVG(lu)   += CWT(nn+1-iq)*U0C(iq,lu);
      UAVGDN(lu) += CWT(nn+1-iq)*U0C(iq,lu);
      FLDN(lu)   += CWT(nn+1-iq)*U0C(iq,lu)*CMU(nn+1-iq);
    }

    for (iq = nn+1; iq <= ds->nstr; iq++) {
      zint = 0.;
      for (jq = 1; jq <= nn; jq++) {
        zint += GC(iq,jq,lyu)*LL(jq,lyu)*exp(-KK(jq,lyu)*(UTAUPR(lu)-TAUCPR(lyu  )));
      }
      for (jq = nn+1; jq <= ds->nstr; jq++) {
        zint += GC(iq,jq,lyu)*LL(jq,lyu)*exp(-KK(jq,lyu)*(UTAUPR(lu)-TAUCPR(lyu-1)));
      }

      U0C(iq,lu) = zint;
      if (ds->bc.fbeam > 0.) {
	if ( ds->flag.spher == TRUE ) {
	  U0C(iq,lu) += exp(-ZBEAMA(lyu)*UTAUPR(lu))*
	    ( ZBEAM0(iq,lyu)+ZBEAM1(iq,lyu)*UTAUPR(lu) );
	}
	else {
	  U0C(iq,lu) += ZZ(iq,lyu)*fact;
	}
      }
      if ( ds->flag.general_source == TRUE ) {
	U0C(iq,lu) += ZZG(iq,lyu);
      }
      U0C(iq,lu) += ZPLK0(iq,lyu)+ZPLK1(iq,lyu)*UTAUPR(lu);
      UAVG(lu)   += CWT(iq-nn)*U0C(iq,lu);
      UAVGUP(lu) += CWT(iq-nn)*U0C(iq,lu);
      FLUP(lu)   += CWT(iq-nn)*U0C(iq,lu)*CMU(iq-nn);
    }
    FLUP(lu)  *= 2.*M_PI;
    FLDN(lu)  *= 2.*M_PI;
    fdntot     = FLDN(lu)+FLDIR(lu);
    fnet       = fdntot-FLUP(lu);
    RFLDN(lu)  = fdntot-RFLDIR(lu);
    UAVG(lu)   = (2.*M_PI*UAVG(lu)+dirint)/(4.*M_PI);
    UAVGSO(lu) =  dirint / (4.*M_PI);
    UAVGDN(lu) = (2.*M_PI*UAVGDN(lu) )/(4.*M_PI);
    UAVGUP(lu) = (2.*M_PI*UAVGUP(lu) )/(4.*M_PI);
    plsorc     = XR0(lyu)+XR1(lyu)*UTAUPR(lu);
    DFDT(lu)   = (1.-SSALB(lyu))*4.*M_PI*(UAVG(lu)-plsorc);

    if (ds->flag.prnt[1]) {
      fprintf(stdout,"%10.4f%7d%12.3e%12.3e%12.3e%12.3e%12.3e%12.3e%12.3e%14.3e\n",
                      UTAU(lu),lyu,RFLDIR(lu),RFLDN(lu),fdntot,FLUP(lu),fnet,UAVG(lu),plsorc,DFDT(lu));
    }
  }

  if (prntu0) {
    fprintf(stdout,"\n\n%s\n"," ******** AZIMUTHALLY AVERAGED INTENSITIES ( at polar quadrature angles ) *******");
    for (lu = 1; lu <= ds->ntau; lu++) {
      fprintf(stdout,"\n%s%10.4f\n\n%s\n",
                     " Optical depth =",UTAU(lu),
                     "     Angle (deg)   cos(Angle)     Intensity     Angle (deg)   cos(Angle)     Intensity");
      for (iq = 1; iq <= nn; iq++) {
        ang1 = acos(CMU(2*nn-iq+1))/DEG;
        ang2 = acos(CMU(     iq  ))/DEG;
        fprintf(stdout,"%16.4f%13.5f%14.3e%16.4f%13.5f%14.3e\n",
                        ang1,CMU(2*nn-iq+1),U0C(iq,   lu),
                        ang2,CMU(     iq  ),U0C(iq+nn,lu));
      }
    }
  }

  return;
}

/*============================= end of c_fluxes() =======================*/

