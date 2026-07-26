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

/*============================= c_print_avg_intensities() ===============*/

/*
   Print azimuthally averaged intensities at user angles

   Called by- c_disort
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_print_avg_intensities(disort_state *ds,
			     disort_output *out)
{
  int
    iu,iumax,iumin,
    lenfmt,lu,np,npass;

  if(ds->numu < 1) {
    return;
  }

  fprintf(stdout,"\n\n *******  AZIMUTHALLY AVERAGED INTENSITIES (at user polar angles)  ********\n");
  lenfmt = 8;
  npass  = 1+(ds->numu-1)/lenfmt;
  fprintf(stdout,"\n   Optical   Polar Angle Cosines"
                 "\n     Depth");

  for (np = 1; np <= npass; np++) {
    iumin = 1+lenfmt*(np-1);
    iumax = IMIN(lenfmt*np,ds->numu);
    fprintf(stdout,"\n          "); 
    for (iu = iumin; iu <= iumax; iu++) {
      fprintf(stdout,"%14.5f",UMU(iu));
    }
    fprintf(stdout,"\n");

    for (lu = 1; lu <= ds->ntau; lu++) {
      fprintf(stdout,"%10.4f",UTAU(lu));
      for (iu = iumin; iu <= iumax; iu++) {
        fprintf(stdout,"%14.4e",U0U(iu,lu));
      }
      fprintf(stdout,"\n");
    }
  }

  return;
}

/*============================= end of c_print_avg_intensities() ========*/

/*============================= c_print_inputs() ========================*/

/*
   Print values of input variables

   Called by- c_disort
 --------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_print_inputs(disort_state *ds,
                    double       *dtaucpr,
                    int           scat_yes,
                    int           deltam,
                    int           corint,
                    double       *flyr,
                    int           lyrcut,
                    double       *oprim,
                    double       *tauc,
                    double       *taucpr)
{
  int
    iq,iu,j,k,lc,lu;

  fprintf(stdout,"\n\n"
                 " ****************************************************************************************************\n"
                 " DISORT: %s\n"
                 " ****************************************************************************************************\n",
                 ds->header);

  fprintf(stdout,"\n No. streams =%4d     No. computational layers =%4d\n",ds->nstr,ds->nlyr);

  if (ds->flag.ibcnd != SPECIAL_BC) {
    fprintf(stdout,"%4d User optical depths :",ds->ntau);
    for (lu = 1; lu <= ds->ntau; lu++) {
      fprintf(stdout,"%10.4f",UTAU(lu));
      if (lu%10 == 0) {
        fprintf(stdout,"\n                          ");
      }
    }
    fprintf(stdout,"\n");
  }

  if (!ds->flag.onlyfl) {
    fprintf(stdout,"%4d User polar angle cosines :",ds->numu);
    for (iu = 1; iu <= ds->numu; iu++) {
      fprintf(stdout,"%9.5f",UMU(iu));
      if (iu%10 == 0) {
        fprintf(stdout,"\n                               ");
      }
    }
    fprintf(stdout,"\n");
  }

  if (!ds->flag.onlyfl && ds->flag.ibcnd != SPECIAL_BC) {
    fprintf(stdout,"%4d User azimuthal angles :",ds->nphi);
    for (j = 1; j <= ds->nphi; j++) {
      fprintf(stdout,"%9.2f",PHI(j));
      if (j%10 == 0) {
        fprintf(stdout,"n                            ");
      }
    }
    fprintf(stdout,"\n");
  }

  if (!ds->flag.planck || ds->flag.ibcnd == SPECIAL_BC) {
    fprintf(stdout," No thermal emission\n");
  }

  if (ds->flag.spher == TRUE) {
    fprintf(stdout," Pseudo-spherical geometry invoked\n");
  }

  if (ds->flag.general_source == TRUE) {
    fprintf(stdout," Calculation with general source term\n");
  }

  if (ds->flag.ibcnd == GENERAL_BC) {
    fprintf(stdout," Boundary condition flag: ds.flag.ibcnd = GENERAL_BC\n");
    fprintf(stdout,"    Incident beam with intensity =%11.3e and polar angle cosine = %8.5f  and azimuth angle =%7.2f\n",
                   ds->bc.fbeam,ds->bc.umu0,ds->bc.phi0);
    fprintf(stdout,"    plus isotropic incident intensity =%11.3e\n",ds->bc.fisot);

    if (ds->bc.fluor > 0.0 ) {
      fprintf(stdout,"    Bottom isotropic exiting intensity =%11.3e\n",ds->bc.fluor);
    }
    if (ds->flag.lamber) {
      fprintf(stdout,"    Bottom albedo (Lambertian) =%8.4f\n",ds->bc.albedo);
    }
    else {
      fprintf(stdout,"    Bidirectional reflectivity at bottom\n");
    }

    if(ds->flag.planck) {
      fprintf(stdout,"    Thermal emission in wavenumber interval :%14.4f%14.4f\n",ds->wvnmlo,ds->wvnmhi);
      fprintf(stdout,"    Bottom temperature =%10.2f    Top temperature =%10.2f    Top emissivity =%8.4f\n",
                     ds->bc.btemp,ds->bc.ttemp,ds->bc.temis);
    }
  }
  else if (ds->flag.ibcnd == SPECIAL_BC) {
    fprintf(stdout," Boundary condition flag: ds.flag.ibcnd = SPECIAL_BC\n");
    fprintf(stdout,"    Isotropic illumination from top and bottom\n");
    fprintf(stdout,"    Bottom albedo (Lambertian) =%8.4f\n",ds->bc.albedo);
  }
  else {
    c_errmsg("Unrecognized ds.flag.ibcnd",DS_WARNING);
  }

  if (deltam) {
    fprintf(stdout," Uses delta-M method\n");
  }
  else {
    fprintf(stdout," Does not use delta-M method\n");
  }

  if (corint) {
    fprintf(stdout," Uses TMS/IMS method\n");
  }
  else {
    fprintf(stdout," Does not use TMS/IMS method\n");
  }

  if (ds->flag.ibcnd == SPECIAL_BC) {
    fprintf(stdout," Calculate albedo and transmissivity of medium vs. incident beam angle\n");
  }
  else if (ds->flag.onlyfl) {
    fprintf(stdout," Calculate fluxes only\n");
  }
  else {
    fprintf(stdout," Calculate fluxes and intensities\n");
  }

  fprintf(stdout," Relative convergence criterion for azimuth series =%11.2e\n",ds->accur);

  if (lyrcut) {
    fprintf(stdout," Sets radiation = 0 below absorption optical depth 10\n");
  }

  /*
   * Print layer variables (to read, skip every other line)
   */
  if(ds->flag.planck) {
    fprintf(stdout,"\n                                     <------------- Delta-M --------------->");
    fprintf(stdout,"\n                   Total    Single                           Total    Single");
    fprintf(stdout,"\n       Optical   Optical   Scatter   Separated   Optical   Optical   Scatter    Asymm");
    fprintf(stdout,"\n         Depth     Depth    Albedo    Fraction     Depth     Depth    Albedo   Factor   Temperature\n");
  }
  else {
    fprintf(stdout,"\n                                     <------------- Delta-M --------------->");
    fprintf(stdout,"\n                   Total    Single                           Total    Single");
    fprintf(stdout,"\n       Optical   Optical   Scatter   Separated   Optical   Optical   Scatter    Asymm");
    fprintf(stdout,"\n         Depth     Depth    Albedo    Fraction     Depth     Depth    Albedo   Factor\n");
  }

  for (lc = 1; lc <= ds->nlyr; lc++) {
    
    if (ds->flag.planck) {
      fprintf(stdout,"%4d%10.4f%10.4f%10.5f%12.5f%10.4f%10.4f%10.5f%9.4f%14.3f\n",
                     lc,DTAUC(lc),TAUC(lc),SSALB(lc),FLYR(lc),DTAUCPR(lc),TAUCPR(lc),OPRIM(lc),PMOM(1,lc),TEMPER(lc-1));
    }
    else {
      fprintf(stdout,"%4d%10.4f%10.4f%10.5f%12.5f%10.4f%10.4f%10.5f%9.4f\n",
                     lc,DTAUC(lc),TAUC(lc),SSALB(lc),FLYR(lc),DTAUCPR(lc),TAUCPR(lc),OPRIM(lc),PMOM(1,lc));
    }
  }
  if (ds->flag.planck) {
    fprintf(stdout,"                                                                                     %14.3f\n",
            TEMPER(ds->nlyr));
  }

  if (ds->flag.prnt[4] && scat_yes) {
    fprintf(stdout,"\n Number of Phase Function Moments = %5d\n",ds->nmom+1);
    fprintf(stdout," Layer   Phase Function Moments\n");
    for (lc = 1; lc <= ds->nlyr; lc++) {
      if (SSALB(lc) > 0.) {
        fprintf(stdout,"%6d",lc);
        for (k = 0; k <= ds->nmom; k++) {
          fprintf(stdout,"%11.6f",PMOM(k,lc));
          if ((k+1)%10 == 0) {
            fprintf(stdout,"\n      ");
          } 
        }
        fprintf(stdout,"\n");
      }
    }
  }

  if (ds->flag.general_source == TRUE) {
    fprintf(stdout," Calculation with general source term\n");
    j = 0;
    for (lc = 1; lc <= ds->nlyr; lc++) {
      fprintf(stdout,"%4d%10.4f",lc,DTAUC(lc));
      for (iq = 1; iq <= ds->nstr; iq++) {	
	fprintf(stdout,"%13.6e",GENSRC(j,lc,iq));
      }
      fprintf(stdout,"\n");
    }
  }


  return;
}

/*============================= end of c_print_inputs() =================*/

/*============================= c_print_intensities() ===================*/

/*
   Prints the intensity at user polar and azimuthal angles
   All arguments are disort state or output variables

   Called by- c_disort
 -------------------------------------------------------------------*/

DISPATCH_MACRO inline void c_print_intensities(disort_state  *ds,
                         disort_output *out)
{
  int
    iu,j,jmax,jmin,lenfmt,lu,np,npass;

  if (ds->nphi < 1) {
    return;
  }

  fprintf(stdout,"\n\n *********  I N T E N S I T I E S  *********\n");
  lenfmt = 10;
  npass  = 1+(ds->nphi-1)/lenfmt;
  fprintf(stdout,"\n             Polar   Azimuth angles (degrees)");
  fprintf(stdout,"\n   Optical   Angle");
  fprintf(stdout,"\n    Depth   Cosine\n");
  for (lu = 1; lu <= ds->ntau; lu++) {
    for (np = 1; np <= npass; np++) {
      jmin = 1+lenfmt*(np-1);
      jmax = IMIN(lenfmt*np,ds->nphi);
      fprintf(stdout,"\n                  ");
      for (j = jmin; j <= jmax; j++) {
        fprintf(stdout,"%11.2f",PHI(j));
      }
      fprintf(stdout,"\n");
      if (np == 1) {
        fprintf(stdout,"%10.4f%8.4f",UTAU(lu),UMU(1));
        for (j = jmin; j <= jmax; j++) {
          fprintf(stdout,"%11.3e",UU(1,lu,j));
        }
        fprintf(stdout,"\n");
      }
      else {
        fprintf(stdout,"          %8.4f",UMU(1));
        for (j = jmin; j <= jmax; j++) {
          fprintf(stdout,"%11.3e",UU(1,lu,j));
        }
        fprintf(stdout,"\n");
      }
      for (iu = 2; iu <= ds->numu; iu++) {
        fprintf(stdout,"          %8.4f",UMU(iu));
        for (j = jmin; j <= jmax; j++) {
          fprintf(stdout,"%11.3e",UU(iu,lu,j));
        }
        fprintf(stdout,"\n");
      }
    }
  }

  return;
}

/*============================= end of c_print_intensities() ============*/

