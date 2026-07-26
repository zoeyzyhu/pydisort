// torch
#include <ATen/Dispatch.h>
#include <ATen/TensorIterator.h>
#include <ATen/native/ReduceOpsUtils.h>
#include <ATen/native/DispatchStub.h>
#include <c10/cuda/CUDAGuard.h>
#include <c10/cuda/CUDAException.h>

// disort
#include <disort/loops.cuh>
#include "disort_dispatch.hpp"
#include "disort_impl.h"

namespace disort {

template <int FastFluxNstr>
void launch_disort_cuda(at::TensorIterator& iter, int upward,
                        disort_state ds0, double* d_wvnmlo,
                        double* d_wvnmhi, double* d_utau, double* d_umu,
                        double* d_phi, size_t work_size,
                        at::Tensor* cuda_workspace) {
  AT_DISPATCH_FLOATING_TYPES(iter.dtype(), "call_disort_cuda", [&] {
    int nprop = (int)at::native::ensure_nonempty_size(iter.input(0), -1);

    native::gpu_chunk_kernel<12>(
        iter, work_size, cuda_workspace,
        [=] GPU_LAMBDA(char* const data[12], const unsigned int strides[12],
                       int64_t idx) {
          auto out = reinterpret_cast<scalar_t*>(data[0] + strides[0]);
          auto prop = reinterpret_cast<scalar_t*>(data[1] + strides[1]);
          auto umu0 = reinterpret_cast<scalar_t*>(data[2] + strides[2]);
          auto phi0 = reinterpret_cast<scalar_t*>(data[3] + strides[3]);
          auto fbeam = reinterpret_cast<scalar_t*>(data[4] + strides[4]);
          auto albedo = reinterpret_cast<scalar_t*>(data[5] + strides[5]);
          auto fluor = reinterpret_cast<scalar_t*>(data[6] + strides[6]);
          auto fisot = reinterpret_cast<scalar_t*>(data[7] + strides[7]);
          auto temis = reinterpret_cast<scalar_t*>(data[8] + strides[8]);
          auto btemp = reinterpret_cast<scalar_t*>(data[9] + strides[9]);
          auto ttemp = reinterpret_cast<scalar_t*>(data[10] + strides[10]);
          auto temf = reinterpret_cast<scalar_t*>(data[11] + strides[11]);

          pmem::pool_init();
          disort_state d = ds0;
          disort_output o{};
          d.wvnmlo = d_wvnmlo[idx];
          d.wvnmhi = d_wvnmhi[idx];
          if constexpr (FastFluxNstr != 0) {
            c_fast_flux_alloc(&d, &o);
          }
          else {
            c_disort_state_alloc(&d);
            c_disort_out_alloc(&d, &o);
          }
          if (d.flag.usrtau) {
            for (int j = 0; j < d.ntau; ++j) d.utau[j] = d_utau[j];
          }
          if (d.flag.usrang) {
            for (int j = 0; j < d.numu; ++j) d.umu[j] = d_umu[j];
            for (int j = 0; j < d.nphi; ++j) d.phi[j] = d_phi[j];
          }

          disort_impl<FastFluxNstr>(out, prop, umu0, phi0, fbeam, albedo,
                                     fluor, fisot, temis, btemp, ttemp, temf,
                                     upward, d, o, nprop);
        });
  });
}

// Native GPU path: one thread runs one (wave, column) element of the
// iterator, executing the same disort_impl -> c_disort code as the CPU
// path with all work memory served by the per-thread pmem pool.
//
// Differences from the CPU path (by design):
//   - results are returned only through the flx output tensor; the host
//     ds_out array is not written, so DisortImpl::gather_flx/gather_rad
//     remain CPU-only.
//   - the emission callback is c_planck_func2, as hard-coded in
//     disort_impl for the CPU path as well.
void call_disort_cuda(at::TensorIterator& iter, int upward, bool force_general,
                      disort_state *ds, disort_output *ds_out,
                      at::Tensor *cuda_workspace) {
  at::cuda::CUDAGuard device_guard(iter.device());
  (void)ds_out;

  int64_t numel = iter.numel();
  if (numel == 0) return;

  // Template state for every element; per-element differences are only
  // wvnmlo/wvnmhi (see DisortImpl::reset), which are gathered below.
  disort_state ds0 = ds[0];

  std::vector<double> wvnmlo(numel), wvnmhi(numel);
  for (int64_t i = 0; i < numel; ++i) {
    wvnmlo[i] = ds[i].wvnmlo;
    wvnmhi[i] = ds[i].wvnmhi;
  }

  // device copies of the host-side per-state arrays filled by reset()
  auto to_device = [](const double *src, size_t n) -> double * {
    if (src == nullptr || n == 0) return nullptr;
    double *dst = nullptr;
    C10_CUDA_CHECK(cudaMalloc(&dst, n * sizeof(double)));
    C10_CUDA_CHECK(
        cudaMemcpy(dst, src, n * sizeof(double), cudaMemcpyHostToDevice));
    return dst;
  };
  double *d_wvnmlo = to_device(wvnmlo.data(), numel);
  double *d_wvnmhi = to_device(wvnmhi.data(), numel);
  double *d_utau =
      ds0.flag.usrtau ? to_device(ds0.utau, ds0.ntau) : nullptr;
  double *d_umu = ds0.flag.usrang ? to_device(ds0.umu, ds0.numu) : nullptr;
  double *d_phi = ds0.flag.usrang ? to_device(ds0.phi, ds0.nphi) : nullptr;

  constexpr size_t kDisortStackBytes = 32 * 1024;
  C10_CUDA_CHECK(cudaDeviceSetLimit(cudaLimitStackSize, kDisortStackBytes));

  bool fast_flux = !force_general && c_fast_flux_eligible(&ds0);
  size_t work_size = fast_flux ? c_fast_flux_work_size(&ds0)
                               : c_disort_work_size(&ds0);
  if (fast_flux && ds0.nstr == 4) {
    launch_disort_cuda<4>(iter, upward, ds0, d_wvnmlo, d_wvnmhi, d_utau,
                          d_umu, d_phi, work_size, cuda_workspace);
  }
  else if (fast_flux) {
    launch_disort_cuda<8>(iter, upward, ds0, d_wvnmlo, d_wvnmhi, d_utau,
                          d_umu, d_phi, work_size, cuda_workspace);
  }
  else {
    launch_disort_cuda<0>(iter, upward, ds0, d_wvnmlo, d_wvnmhi, d_utau,
                          d_umu, d_phi, work_size, cuda_workspace);
  }

  C10_CUDA_CHECK(cudaFree(d_wvnmlo));
  C10_CUDA_CHECK(cudaFree(d_wvnmhi));
  C10_CUDA_CHECK(cudaFree(d_utau));
  C10_CUDA_CHECK(cudaFree(d_umu));
  C10_CUDA_CHECK(cudaFree(d_phi));
}

}  // namespace disort

namespace at::native {

REGISTER_CUDA_DISPATCH(call_disort, &disort::call_disort_cuda);

}  // namespace at::native
