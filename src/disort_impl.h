#pragma once

// disort
#include <cdisort213/cdisort.h>

// disort
#include "common.h"

#define FLX(i, m) flx[(i) * 2 + (m)]
#define PROP(i, m) prop[(i) * nprop + (m)]
#define FBEAM (*fbeam)
#define UMU0 (*umu0)
#define PHI0 (*phi0)
#define ALBEDO (*albedo)
#define FLUOR (*fluor)
#define FISOT (*fisot)
#define TEMF(i) temf[i]

namespace disort {

template <typename T>
void disort_impl(T* flx, T* prop, T* fbeam, T* umu0, T* phi0, T* albedo,
                 T* fluor, T* fisot, T* temf, int rank_in_column,
                 disort_state& ds, disort_output& ds_out, int nprop) {
  // run disort
  if (ds.flag.planck) {
    for (int i = 0; i <= ds.nlyr; ++i) {
      ds.temper[ds.nlyr - i] = TEMF(i);
    }
  }

  // bc
  ds.bc.fbeam = FBEAM;
  ds.bc.umu0 = UMU0;
  ds.bc.phi0 = PHI0;
  ds.bc.albedo = ALBEDO;
  ds.bc.fluor = FLUOR;
  ds.bc.fisot = FISOT;

  for (int i = 0; i < ds.nlyr; ++i) {
    // absorption
    ds.dtauc[ds.nlyr - 1 - i] = PROP(i, index::IAB);

    // single scatering albedo
    ds.ssalb[ds.nlyr - 1 - i] = PROP(i, index::ISS);

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

  c_disort(&ds, &ds_out);

  for (int i = 0; i <= ds.nlyr; ++i) {
    int i1 = ds.nlyr - (rank_in_column * (ds.nlyr - 1) + i);
    FLX(i, index::IUP) = ds_out.rad[i1].flup;
    FLX(i, index::IDN) = ds_out.rad[i1].rfldir + ds_out.rad[i1].rfldn;
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
#undef TEMF
