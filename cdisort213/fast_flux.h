/* Flux-only CUDA path for the common plane-parallel Lambertian case.
 *
 * This retains the DISORT eigenproblem, boundary system, and flux equations.
 * It only omits storage and code paths which cannot contribute when the
 * caller has selected onlyfl, a Lambertian surface, and no pseudo-spherical
 * or general source terms.
 */
#pragma once

#include "cdisort.h"
#include "pmem.h"

DISPATCH_MACRO inline int c_fast_flux_eligible(const disort_state *ds)
{
  return (ds->nstr == 4 || ds->nstr == 8) && ds->flag.onlyfl &&
         ds->flag.lamber && !ds->flag.spher &&
         !ds->flag.general_source && !ds->flag.usrtau &&
         !ds->flag.usrang && !ds->flag.output_uum;
}

DISPATCH_MACRO inline void c_fast_flux_alloc(disort_state *ds,
                                             disort_output *out)
{
  ds->fast_flux = TRUE;
  ds->nmom_nstr = IMAX(ds->nmom, ds->nstr);
  ds->dtauc = c_dbl_vector_uninitialized(0, ds->nlyr, "ds->dtauc");
  ds->ssalb = c_dbl_vector_uninitialized(0, ds->nlyr, "ds->ssalb");
  ds->pmom = c_dbl_vector_uninitialized(
      0, (ds->nmom_nstr + 1) * ds->nlyr - 1, "ds->pmom");
  ds->temper = ds->flag.planck
                   ? c_dbl_vector_uninitialized(0, ds->nlyr, "ds->temper")
                   : NULL;
  ds->gensrc = NULL;
  ds->gensrcu = NULL;
  ds->ntau = ds->nlyr + 1;
  ds->utau = c_dbl_vector_uninitialized(0, ds->ntau - 1, "ds->utau");
  ds->zd = NULL;
  ds->numu = ds->nstr;
  ds->umu = c_dbl_vector_uninitialized(0, ds->numu - 1, "ds->umu");
  ds->phi = NULL;
  ds->mu_phase = NULL;
  ds->phase = NULL;

  out->rad = (disort_radiant *)pcalloc(ds->ntau, sizeof(disort_radiant));
  out->uu = NULL;
  out->u0u = NULL;
  out->uum = NULL;
  out->albmed = NULL;
  out->trnmed = NULL;
}

/* Maximum extra storage used when the 8-stream block solve falls through to
 * c_solve0.  The CUDA pool is a bump allocator, so both block workspaces can
 * be live in that case. */
DISPATCH_MACRO inline size_t c_fast_flux_work_size(const disort_state *ds)
{
  size_t bytes = c_disort_work_size(ds);
  if (ds->nstr == 8) {
    const size_t block = (size_t)(3 * ds->nlyr - 2) * 64 * sizeof(double);
    bytes += block + (size_t)ds->nstr * ds->nlyr * sizeof(double);
    if (!ds->flag.planck) bytes += block;
  }
  return (bytes + 255) & ~(size_t)255;
}

DISPATCH_MACRO inline void c_fast_fluxes(disort_state *ds,
                                         disort_output *out,
                                         double *cmu, double *cwt,
                                         double *gc, double *kk, int *layru,
                                         double *ll, int lyrcut, int ncut,
                                         int nn, double *taucpr,
                                         double *utaupr, double *zz,
                                         disort_pair *plk)
{
  for (int lu = 1; lu <= ds->ntau; ++lu) {
    int lyu = LAYRU(lu);
    if (lyrcut && lyu > ncut) continue;

    double fact = 0.;
    double fldir = 0.;
    double rfldir = 0.;
    if (ds->bc.fbeam > 0.) {
      fact = exp(-UTAUPR(lu) / ds->bc.umu0);
      fldir = ds->bc.umu0 * ds->bc.fbeam * fact;
      rfldir = ds->bc.umu0 * ds->bc.fbeam *
               exp(-UTAU(lu) / ds->bc.umu0);
    }

    double fldn = 0.;
    for (int iq = 1; iq <= nn; ++iq) {
      double zint = 0.;
      for (int jq = 1; jq <= nn; ++jq) {
        zint += GC(iq, jq, lyu) * LL(jq, lyu) *
                exp(-KK(jq, lyu) * (UTAUPR(lu) - TAUCPR(lyu)));
      }
      for (int jq = nn + 1; jq <= ds->nstr; ++jq) {
        zint += GC(iq, jq, lyu) * LL(jq, lyu) *
                exp(-KK(jq, lyu) * (UTAUPR(lu) - TAUCPR(lyu - 1)));
      }
      if (ds->bc.fbeam > 0.) zint += ZZ(iq, lyu) * fact;
      if (ds->flag.planck)
        zint += ZPLK0(iq, lyu) + ZPLK1(iq, lyu) * UTAUPR(lu);
      fldn += CWT(nn + 1 - iq) * zint * CMU(nn + 1 - iq);
    }

    double flup = 0.;
    for (int iq = nn + 1; iq <= ds->nstr; ++iq) {
      double zint = 0.;
      for (int jq = 1; jq <= nn; ++jq) {
        zint += GC(iq, jq, lyu) * LL(jq, lyu) *
                exp(-KK(jq, lyu) * (UTAUPR(lu) - TAUCPR(lyu)));
      }
      for (int jq = nn + 1; jq <= ds->nstr; ++jq) {
        zint += GC(iq, jq, lyu) * LL(jq, lyu) *
                exp(-KK(jq, lyu) * (UTAUPR(lu) - TAUCPR(lyu - 1)));
      }
      if (ds->bc.fbeam > 0.) zint += ZZ(iq, lyu) * fact;
      if (ds->flag.planck)
        zint += ZPLK0(iq, lyu) + ZPLK1(iq, lyu) * UTAUPR(lu);
      flup += CWT(iq - nn) * zint * CMU(iq - nn);
    }

    FLUP(lu) = 2. * M_PI * flup;
    RFLDIR(lu) = rfldir;
    RFLDN(lu) = 2. * M_PI * fldn + fldir - rfldir;
  }
}

DISPATCH_MACRO inline void c_fast_flux_block_set8(double *diag,
                                                   double *lower,
                                                   double *upper,
                                                   int nblock, int row,
                                                   int col, double value)
{
  if (row < 1 || row > 8 * nblock) return;
  int row_block = (row - 1) / 8;
  int col_block = (col - 1) / 8;
  int row_local = (row - 1) % 8;
  int col_local = (col - 1) % 8;
  if (row_block == col_block) {
    diag[(size_t)row_block * 64 + row_local + col_local * 8] = value;
  }
  else if (row_block == col_block + 1) {
    lower[(size_t)col_block * 64 + row_local + col_local * 8] = value;
  }
  else if (col_block == row_block + 1) {
    upper[(size_t)row_block * 64 + row_local + col_local * 8] = value;
  }
}

/* Build the nstr=8 Lambertian flux boundary system directly as contiguous
 * block-tridiagonal matrices.  This is algebraically the c_set_matrix()
 * construction, without its sparse LINPACK-band intermediate. */
DISPATCH_MACRO inline void c_fast_flux_set_blocks8(
    disort_state *ds, double *bdr, double *cmu, double *cwt,
    double *dtaucpr, double *gc, double *kk, int ncut, double *taucpr,
    double *wk, double *work)
{
  const int nn = 4;
  const int nblock = ncut;
  const int nshift = 19;
  const int middle = 23;
  double *diag = work;
  double *lower = diag + (size_t)nblock * 64;
  double *upper = lower + (size_t)(nblock - 1) * 64;
  memset(diag, 0, (size_t)nblock * 64 * sizeof(double));
  if (nblock > 1) {
    memset(lower, 0, (size_t)(nblock - 1) * 64 * sizeof(double));
    memset(upper, 0, (size_t)(nblock - 1) * 64 * sizeof(double));
  }

  for (int lc = 1; lc <= ncut; ++lc) {
    for (int iq = 1; iq <= nn; ++iq) {
      WK(iq) = exp(KK(iq, lc) * DTAUCPR(lc));
    }
    int jcol = 0;
    for (int iq = 1; iq <= nn; ++iq) {
      int col = (lc - 1) * 8 + iq;
      int irow = nshift - jcol;
      for (int jq = 1; jq <= 8; ++jq) {
        c_fast_flux_block_set8(diag, lower, upper, nblock,
                               col + irow + 8 - middle, col,
                               GC(jq, iq, lc));
        c_fast_flux_block_set8(diag, lower, upper, nblock,
                               col + irow - middle, col,
                               -GC(jq, iq, lc) * WK(iq));
        ++irow;
      }
      ++jcol;
    }
    for (int iq = nn + 1; iq <= 8; ++iq) {
      int col = (lc - 1) * 8 + iq;
      int irow = nshift - jcol;
      for (int jq = 1; jq <= 8; ++jq) {
        c_fast_flux_block_set8(diag, lower, upper, nblock,
                               col + irow + 8 - middle, col,
                               GC(jq, iq, lc) * WK(9 - iq));
        c_fast_flux_block_set8(diag, lower, upper, nblock,
                               col + irow - middle, col,
                               -GC(jq, iq, lc));
        ++irow;
      }
      ++jcol;
    }
  }

  int jcol = 0;
  for (int iq = 1; iq <= nn; ++iq) {
    int col = iq;
    int irow = nshift - jcol + nn;
    double expa = exp(KK(iq, 1) * TAUCPR(1));
    for (int jq = nn; jq >= 1; --jq) {
      c_fast_flux_block_set8(diag, lower, upper, nblock,
                             col + irow - middle, col,
                             GC(jq, iq, 1) * expa);
      ++irow;
    }
    ++jcol;
  }
  for (int iq = nn + 1; iq <= 8; ++iq) {
    int col = iq;
    int irow = nshift - jcol + nn;
    for (int jq = nn; jq >= 1; --jq) {
      c_fast_flux_block_set8(diag, lower, upper, nblock,
                             col + irow - middle, col, GC(jq, iq, 1));
      ++irow;
    }
    ++jcol;
  }

  int nncol = 8 * nblock - 8;
  jcol = 0;
  for (int iq = 1; iq <= nn; ++iq) {
    int col = ++nncol;
    int irow = nshift - jcol + 8;
    for (int jq = nn + 1; jq <= 8; ++jq) {
      double sum = 0.;
      for (int k = 1; k <= nn; ++k) {
        sum += CWT(k) * CMU(k) * BDR(jq - nn, k) *
               GC(nn + 1 - k, iq, ncut);
      }
      c_fast_flux_block_set8(diag, lower, upper, nblock,
                             col + irow - middle, col,
                             GC(jq, iq, ncut) - 2. * sum);
      ++irow;
    }
    ++jcol;
  }
  for (int iq = nn + 1; iq <= 8; ++iq) {
    int col = ++nncol;
    int irow = nshift - jcol + 8;
    double expa = WK(9 - iq);
    for (int jq = nn + 1; jq <= 8; ++jq) {
      double sum = 0.;
      for (int k = 1; k <= nn; ++k) {
        sum += CWT(k) * CMU(k) * BDR(jq - nn, k) *
               GC(nn + 1 - k, iq, ncut);
      }
      c_fast_flux_block_set8(diag, lower, upper, nblock,
                             col + irow - middle, col,
                             (GC(jq, iq, ncut) - 2. * sum) * expa);
      ++irow;
    }
    ++jcol;
  }
}

DISPATCH_MACRO inline int c_fast_flux_solve_blocks8(
    disort_state *ds, double *b, double *bdr, double *cmu, double *cwt,
    double *expbea, int *ipvt, double *ll, int ncut, double *zz,
    double *work)
{
  const int nn = 4;
  const int ncol = 8 * ncut;
  memset(b, 0, (size_t)ncol * sizeof(double));
  if (ds->bc.fbeam == 0.) {
    for (int iq = 1; iq <= nn; ++iq) B(iq) = ds->bc.fisot;
  }
  else {
    for (int iq = 1; iq <= nn; ++iq) {
      B(iq) = -ZZ(nn + 1 - iq, 1) + ds->bc.fisot;
    }
    for (int iq = 1; iq <= nn; ++iq) {
      double sum = 0.;
      for (int jq = 1; jq <= nn; ++jq) {
        sum += CWT(jq) * CMU(jq) * BDR(iq, jq) *
               ZZ(nn + 1 - jq, ncut) * EXPBEA(ncut);
      }
      B(ncol - nn + iq) =
          2. * sum +
          (BDR(iq, 0) * ds->bc.umu0 * ds->bc.fbeam / M_PI -
           ZZ(iq + nn, ncut)) * EXPBEA(ncut) +
          ds->bc.fluor;
    }
    int it = nn;
    for (int lc = 1; lc < ncut; ++lc) {
      for (int iq = 1; iq <= 8; ++iq) {
        B(++it) = (ZZ(iq, lc + 1) - ZZ(iq, lc)) * EXPBEA(lc);
      }
    }
  }

  if (!c_solve_block_tridiag8_blocks(ncut, b, ipvt, work)) return FALSE;
  for (int lc = 1; lc <= ncut; ++lc) {
    int ipnt = lc * 8 - nn;
    for (int iq = 1; iq <= nn; ++iq) {
      LL(nn - iq + 1, lc) = B(ipnt - iq + 1);
      LL(nn + iq, lc) = B(ipnt + iq);
    }
  }
  return TRUE;
}

template <int NSTR>
DISPATCH_MACRO inline int c_fast_flux(disort_state *ds, disort_output *out)
{
  static_assert(NSTR == 4 || NSTR == 8);
  ds->fast_flux = TRUE;
  int iq, k, lc, lyrcut, ncol, ncut, nn;
  int *ipvt, *layru;
  double delm0, abstau;
  double *array, *b, *bdr, *bem, *block_work = NULL, *cband, *cc, *ch;
  double *chtau;
  double *cmu, *cwt, *dtaucpr, *eval, *evecc, *expbea, *flyr;
  double *gc, *gl, *kk, *ll, *oprim, *tauc, *taucpr;
  double *utaupr, *wk, *ylm0, *ylmc, *zj, *zz, *zzg;
  disort_pair *ab, *plk, *xr, *zee;
  double bplanck = 0.;
  double tplanck = 0.;
  double *pkag = NULL;

  ipvt = (int *)pmalloc(ds->nstr * ds->nlyr * sizeof(int));
  layru = (int *)pmalloc(ds->ntau * sizeof(int));
  tauc = c_dbl_vector_uninitialized(0, ds->nlyr, "tauc");
  array = c_dbl_vector_uninitialized(0, ds->nstr * ds->nstr - 1, "array");
  b = c_dbl_vector_uninitialized(0, ds->nstr * ds->nlyr - 1, "b");
  bdr = c_dbl_vector_uninitialized(
      0, ((ds->nstr / 2) + 1) * (ds->nstr / 2) - 1, "bdr");
  if constexpr (NSTR == 8) {
    if (!ds->flag.planck) {
      block_work = c_dbl_vector_uninitialized(
          0, (3 * ds->nlyr - 2) * 64 - 1, "block_work");
    }
  }
  cc = c_dbl_vector_uninitialized(0, ds->nstr * ds->nstr - 1, "cc");
  ch = c_dbl_vector_uninitialized(0, ds->nlyr - 1, "ch");
  chtau = c_dbl_vector_uninitialized(0, 2 * ds->nlyr, "chtau");
  cmu = c_dbl_vector_uninitialized(0, ds->nstr - 1, "cmu");
  cwt = c_dbl_vector_uninitialized(0, ds->nstr - 1, "cwt");
  dtaucpr = c_dbl_vector_uninitialized(0, ds->nlyr - 1, "dtaucpr");
  eval = c_dbl_vector_uninitialized(0, (ds->nstr / 2) - 1, "eval");
  evecc = c_dbl_vector_uninitialized(0, ds->nstr * ds->nstr - 1, "evecc");
  expbea = c_dbl_vector_uninitialized(0, ds->nlyr, "expbea");
  flyr = c_dbl_vector_uninitialized(0, ds->nlyr, "flyr");
  gc = c_dbl_vector_uninitialized(0, ds->nlyr * ds->nstr * ds->nstr - 1, "gc");
  gl = c_dbl_vector_uninitialized(0, ds->nlyr * (ds->nstr + 1) - 1, "gl");
  kk = c_dbl_vector_uninitialized(0, ds->nlyr * ds->nstr - 1, "kk");
  ll = c_dbl_vector_uninitialized(0, ds->nlyr * ds->nstr - 1, "ll");
  oprim = c_dbl_vector_uninitialized(0, ds->nlyr - 1, "oprim");
  taucpr = c_dbl_vector_uninitialized(0, ds->nlyr, "taucpr");
  utaupr = c_dbl_vector_uninitialized(0, ds->ntau - 1, "utaupr");
  wk = c_dbl_vector_uninitialized(0, ds->nstr - 1, "wk");
  ylm0 = c_dbl_vector_uninitialized(0, ds->nstr, "ylm0");
  ylmc = c_dbl_vector_uninitialized(0, ds->nstr * (ds->nstr + 1) - 1, "ylmc");
  zj = c_dbl_vector_uninitialized(0, ds->nstr - 1, "zj");
  zz = c_dbl_vector_uninitialized(0, ds->nlyr * ds->nstr - 1, "zz");
  ab = (disort_pair *)pmalloc((ds->nstr / 2) * (ds->nstr / 2) *
                              sizeof(disort_pair));
  xr = ds->flag.planck
           ? (disort_pair *)pcalloc(ds->nlyr, sizeof(disort_pair))
           : NULL;
  zee = ds->flag.planck
            ? (disort_pair *)pcalloc(ds->nstr, sizeof(disort_pair))
            : NULL;
  plk = ds->flag.planck
            ? (disort_pair *)pcalloc(ds->nlyr * ds->nstr,
                                     sizeof(disort_pair))
            : NULL;
  if (ds->flag.planck) {
    pkag = c_dbl_vector_uninitialized(0, ds->nlyr, "pkag");
  }

  TAUC(0) = 0.;
  abstau = 0.;
  for (lc = 1; lc <= ds->nlyr; ++lc) {
    if (SSALB(lc) == 1.) SSALB(lc) = 1. - 100. * DBL_EPSILON;
    TAUC(lc) = TAUC(lc - 1) + DTAUC(lc);
    abstau += (1. - SSALB(lc)) * DTAUC(lc);
  }

  if (!ds->flag.planck && ds->bc.fbeam > 0. && ds->bc.umu0 > 0. &&
      ds->bc.fisot == 0. &&
      ds->bc.fluor == 0.) {
    int pure_absorption = TRUE;
    for (lc = 1; lc <= ds->nlyr; ++lc) {
      if (SSALB(lc) != 0.) {
        pure_absorption = FALSE;
        break;
      }
    }
    if (pure_absorption) {
      double cmu[NSTR / 2], cwt[NSTR / 2];
      c_gaussian_quadrature(NSTR / 2, cmu, cwt);
      const double bottom_direct = ds->bc.umu0 * ds->bc.fbeam *
          exp(-TAUC(ds->nlyr) / ds->bc.umu0);
      int ncut = ds->nlyr;
      double absorption = 0.;
      for (lc = 1; lc <= ds->nlyr; ++lc) {
        if (absorption < 10.) ncut = lc;
        absorption += DTAUC(lc);
      }
      const int lyrcut = absorption >= 10. && ds->nlyr > 1;
      for (int lu = 1; lu <= ds->ntau; ++lu) {
        if (lyrcut && lu - 1 > ncut) continue;
        const double tau = TAUC(lu - 1);
        double transmission = 0.;
        for (int iq = 0; iq < NSTR / 2; ++iq) {
          transmission += 2. * cwt[iq] * cmu[iq] *
              exp(-(TAUC(ds->nlyr) - tau) / cmu[iq]);
        }
        RFLDIR(lu) = ds->bc.umu0 * ds->bc.fbeam *
            exp(-tau / ds->bc.umu0);
        RFLDN(lu) = 0.;
        FLUP(lu) = ds->bc.albedo * bottom_direct * transmission;
      }
      return 0;
    }
  }

  int deltam = TRUE;
  for (lc = 1; lc <= ds->nlyr; ++lc) {
    if (PMOM(ds->nstr, lc) != 0.) break;
    if (lc == ds->nlyr) deltam = FALSE;
  }

  int corint = FALSE;
  c_disort_set(ds, ch, chtau, cmu, cwt, deltam, dtaucpr, expbea, flyr, gl,
               layru, &lyrcut, &ncut, &nn, &corint, oprim, tauc, taucpr,
               utaupr, c_planck_func2);
  if (lyrcut) {
    memset(out->rad, 0, (size_t)ds->ntau * sizeof(disort_radiant));
  }

  if (ds->flag.planck) {
    tplanck = c_planck_func2(ds->wvnmlo, ds->wvnmhi, ds->bc.ttemp) *
              ds->bc.temis;
    bplanck = c_planck_func2(ds->wvnmlo, ds->wvnmhi, ds->bc.btemp);
    for (int lev = 0; lev <= ds->nlyr; ++lev) {
      PKAG(lev) = c_planck_func2(ds->wvnmlo, ds->wvnmhi, TEMPER(lev));
    }
  }

  delm0 = 1.;
  if (!lyrcut) {
    memset(bdr, 0, (ds->nstr / 2) * ((ds->nstr / 2) + 1) * sizeof(double));
    for (iq = 1; iq <= nn; ++iq) {
      for (int jq = 0; jq <= nn; ++jq) {
        BDR(iq, jq) = ds->bc.albedo;
      }
    }
  }
  double beam_cosine = -ds->bc.umu0;
  c_legendre_poly(1, 0, ds->nstr, ds->nstr - 1, &beam_cosine, ylm0);
  c_legendre_poly(nn, 0, ds->nstr, ds->nstr - 1, cmu, ylmc);
  for (k = 0; k <= ds->nstr - 1; ++k) {
    for (iq = nn + 1; iq <= ds->nstr; ++iq) {
      YLMC(k, iq) = ((k & 1) ? -1. : 1.) * YLMC(k, iq - nn);
    }
  }

  for (lc = 1; lc <= ncut; ++lc) {
    if constexpr (NSTR == 4) {
      c_solve_eigen4(ds, lc, ab, array, cmu, cwt, gl, ylmc, cc, evecc, eval,
                     kk, gc, wk);
    }
    else {
      c_solve_eigen8(ds, lc, ab, array, cmu, cwt, gl, ylmc, cc, evecc, eval,
                     kk, gc, wk);
    }
    if (ds->bc.fbeam > 0.) {
      c_upbeam(ds, lc, array, cc, cmu, delm0, gl, ipvt, 0, nn, wk, ylm0,
               ylmc, zj, zz);
    }
    if (ds->flag.planck) {
      XR1(lc) = 0.;
      if (DTAUCPR(lc) > 1.e-4) {
        XR1(lc) = (PKAG(lc) - PKAG(lc - 1)) / DTAUCPR(lc);
      }
      XR0(lc) = PKAG(lc - 1) - XR1(lc) * TAUCPR(lc - 1);
      c_upisot(ds, lc, array, cc, cmu, ipvt, nn, oprim, wk, xr, zee, plk);
    }
  }

  if constexpr (NSTR == 8) {
    if (!ds->flag.planck && !lyrcut && ncut == ds->nlyr) {
      c_fast_flux_set_blocks8(ds, bdr, cmu, cwt, dtaucpr, gc, kk, ncut,
                              taucpr, wk, block_work);
      if (c_fast_flux_solve_blocks8(ds, b, bdr, cmu, cwt, expbea, ipvt, ll,
                                    ncut, zz, block_work)) {
        c_fast_fluxes(ds, out, cmu, cwt, gc, kk, layru, ll, lyrcut, ncut, nn,
                      taucpr, utaupr, zz, plk);
        return 0;
      }
    }
  }

  {
    bem = c_dbl_vector_uninitialized(0, (ds->nstr / 2) - 1, "bem");
    if (!lyrcut) {
      for (int iq = 1; iq <= nn; ++iq) BEM(iq) = 1. - ds->bc.albedo;
    }
    zzg = c_dbl_vector(0, ds->nlyr * ds->nstr - 1, "zzg");
    if (!ds->flag.planck) {
      plk = (disort_pair *)pcalloc(ds->nlyr * ds->nstr,
                                   sizeof(disort_pair));
    }
    cband = c_dbl_vector(
        0, ds->nstr * ds->nlyr * (9 * (ds->nstr / 2) - 2) - 1, "cband");
    c_set_matrix(ds, bdr, cband, cmu, cwt, delm0, dtaucpr, gc, kk, lyrcut,
                 &ncol, ncut, taucpr, wk);
    c_solve0(ds, b, bdr, bem, bplanck, cband, cmu, cwt, expbea, ipvt, ll,
             lyrcut, 0, ncol, ncut, nn, tplanck, taucpr, NULL, NULL, NULL,
             zz, zzg, plk);
  }
  c_fast_fluxes(ds, out, cmu, cwt, gc, kk, layru, ll, lyrcut, ncut, nn,
                taucpr, utaupr, zz, plk);

  return 0;
}
