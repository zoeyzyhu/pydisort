#pragma once

// disort
#include <cdisort213/cdisort.h>
#include <disort/index.h>

#define FLX(i, m) flx[(i) * 2 + (m)]
#define PROP(i, m) prop[(i) * nprop + (m)]
#define FBEAM (*fbeam)
#define UMU0 (*umu0)
#define PHI0 (*phi0)
#define ALBEDO (*albedo)
#define FLUOR (*fluor)
#define FISOT (*fisot)
#define TEMIS (*temis)
#define BTEMP (*btemp)
#define TTEMP (*ttemp)
#define TEMF(i) temf[i]

namespace disort {

template <typename T>
void disort_impl(T *flx, T *prop, T *umu0, T *phi0, T *fbeam, T *albedo,
                 T *fluor, T *fisot, T *temis, T *btemp, T *ttemp, T *temf,
                 int upward, disort_state &ds, disort_output &ds_out,
                 int nprop) {
  // run disort
  if (ds.flag.planck) {
    if (upward) {
      for (int i = 0; i <= ds.nlyr; ++i) {
        ds.temper[ds.nlyr - i] = TEMF(i);
      }
    } else {
      for (int i = 0; i <= ds.nlyr; ++i) {
        ds.temper[i] = TEMF(i);
      }
    }
  }

  // bc
  ds.bc.umu0 = UMU0;
  ds.bc.phi0 = PHI0;
  ds.bc.fbeam = FBEAM;
  ds.bc.albedo = ALBEDO;
  ds.bc.fluor = FLUOR;
  ds.bc.fisot = FISOT;
  ds.bc.temis = TEMIS;
  ds.bc.btemp = BTEMP;
  ds.bc.ttemp = TTEMP;

  if (upward) {
    for (int i = 0; i < ds.nlyr; ++i) {
      // absorption
      ds.dtauc[ds.nlyr - 1 - i] = PROP(i, index::IEX);

      // single scatering albedo
      if (nprop > 1) {
        ds.ssalb[ds.nlyr - 1 - i] = PROP(i, index::ISS);
      } else {
        ds.ssalb[ds.nlyr - 1 - i] = 0.;
      }

      // Legendre coefficients
      ds.pmom[(ds.nlyr - 1 - i) * (ds.nmom_nstr + 1)] = 1.;
      for (int m = 0; m < nprop - 2; ++m) {
        ds.pmom[(ds.nlyr - 1 - i) * (ds.nmom_nstr + 1) + m + 1] =
            PROP(i, index::IPM + m);
      }

      for (int m = nprop - 2; m < ds.nmom; ++m) {
        ds.pmom[(ds.nlyr - 1 - i) * (ds.nmom_nstr + 1) + m + 1] = 0.;
      }
    }
  } else {
    for (int i = 0; i < ds.nlyr; ++i) {
      // absorption
      ds.dtauc[i] = PROP(i, index::IEX);

      // single scatering albedo
      if (nprop > 1) {
        ds.ssalb[i] = PROP(i, index::ISS);
      } else {
        ds.ssalb[i] = 0.;
      }

      // Legendre coefficients
      ds.pmom[i * (ds.nmom_nstr + 1)] = 1.;
      for (int m = 0; m < nprop - 2; ++m) {
        ds.pmom[i * (ds.nmom_nstr + 1) + m + 1] = PROP(i, index::IPM + m);
      }

      for (int m = nprop - 2; m < ds.nmom; ++m) {
        ds.pmom[i * (ds.nmom_nstr + 1) + m + 1] = 0.;
      }
    }
  }

  c_disort(&ds, &ds_out, c_planck_func2);

  if (upward) {
    for (int i = 0; i < ds.ntau; ++i) {
      FLX(ds.ntau - 1 - i, index::IUP) = ds_out.rad[i].flup;
      FLX(ds.ntau - 1 - i, index::IDN) =
          ds_out.rad[i].rfldir + ds_out.rad[i].rfldn;
    }
  } else {
    for (int i = 0; i < ds.ntau; ++i) {
      FLX(i, index::IUP) = ds_out.rad[i].flup;
      FLX(i, index::IDN) = ds_out.rad[i].rfldir + ds_out.rad[i].rfldn;
    }
  }
}

}  // namespace disort

#undef FLX
#undef PROP
#undef FTOA
#undef FBEAM
#undef UMU0
#undef PHI0
#undef ALBEDO
#undef FLUOR
#undef FISOT
#undef BTEMP
#undef TTEMP
#undef TEMIS
#undef TEMF
